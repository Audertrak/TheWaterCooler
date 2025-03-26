import socket
import json
import threading
import time
from typing import Callable, Dict, Tuple, Optional
from zeroconf import Zeroconf, ServiceInfo, ServiceBrowser, ServiceStateChange
from protocols.base import ProtocolBase
from peer import PeerManager

from message.base import MessageBase
from message.json import JSONMessage
from message.protobuf import SimpleProtobufMessage
from message.mqtt import MQTTMessage
from message.raw import RawMessage

class MDNSProtocol(ProtocolBase):
    def __init__(self, peer_id: str, on_peer_discovered: Callable, on_message: Callable,
                 service_name: str = "_p2ptester._tcp.local.", port: int = 5558,
                 message_format: Optional[MessageBase] = None,
                 peer_manager: Optional[PeerManager] = None):
        super().__init__(peer_id, on_peer_discovered, on_message, peer_manager)
        self.service_type = service_name
        self.port = port
        self.message_format = message_format or JSONMessage()
        
        self.zeroconf = None
        self.browser = None
        self.service_info = None
        self.server_socket = None
        self.client_sockets = {}

    def _run(self):
        """Main mDNS listener and advertising loop"""
        try:
            self._start_tcp_server()
            self._start_zeroconf()
            while self.running:
                time.sleep(1)
            
        except Exception as e:
            self.log(f"mDNS error: {str(e)}")
        finally:
            self._cleanup()

    def _start_zeroconf(self):
        """Initialize and start Zeroconf service"""
        try:
            self.zeroconf = Zeroconf()
            
            # Create service info
            self.service_info = ServiceInfo(
                type_=self.service_type,
                name=f"{self.peer_id}.{self.service_type}",
                addresses=[socket.inet_aton(self._get_local_ip())],
                port=self.port,
                properties={
                    'peer_id': self.peer_id,
                    'version': '1.0'
                },
                server=f"{self.peer_id}.local."
            )
            
            # Register our service
            self.zeroconf.register_service(self.service_info)
            
            # Browse for other services
            self.browser = ServiceBrowser(
                self.zeroconf,
                self.service_type,
                handlers=[self._on_service_state_change]
            )
            
            self.log("mDNS service started")

        except Exception as e:
            self.log(f"Error starting Zeroconf: {str(e)}")
            if self.zeroconf:
                self.zeroconf.close()
            self.zeroconf = None
            self.service_info = None
            raise
        
    def _on_service_state_change(self, 
                                zeroconf: Zeroconf,
                                service_type: str,
                                name: str,
                                state_change: ServiceStateChange):
        """Handle service discovery events"""
        if state_change in [ServiceStateChange.Added, ServiceStateChange.Updated]:
            info = zeroconf.get_service_info(service_type, name)
            if info:
                properties = info.properties
                peer_id = properties.get(b'peer_id', b'').decode('utf-8')
                
                if peer_id and peer_id != self.peer_id:
                    # Use PeerManager to track peer
                    peer = self.peer_manager.add_or_update_peer(
                        peer_id,
                        "mdns",
                        ip=socket.inet_ntoa(info.addresses[0]),
                        port=info.port
                    )
                    self.log(f"Discovered peer: {peer_id}")
                    self.on_peer_discovered(peer_id, "mdns")
                    
    def _start_tcp_server(self):
        """Start TCP server for direct communication"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('', self.port))
            self.server_socket.listen(5)
            self.server_socket.settimeout(1.0)
            # Start server thread
            server_thread = threading.Thread(target=self._run_tcp_server)
            server_thread.daemon = True
            server_thread.start()

        except Exception as e:
            self.log(f"Error starting TCP server: {str(e)}")
            if self.server_socket:
                self.server_socket.close()
            self.server_socket = None
        
    def _run_tcp_server(self):
        """Run TCP server loop"""
        while self.running and self.server_socket:
            try:
                client_socket, addr = self.server_socket.accept()
                client_thread = threading.Thread(
                    target=self._handle_client,
                    args=(client_socket, addr)
                )
                client_thread.daemon = True
                client_thread.start()
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    self.log(f"TCP server error: {str(e)}")
                break
                    
    def _handle_client(self, client_socket: socket.socket, addr: tuple):
        """Handle incoming TCP connection"""
        peer_id = None
        try:
            client_socket.settimeout(10.0)
            
            # First message should be peer identification
            data = client_socket.recv(4096)
            if not data:
                return
                
            msg = self.message_format.decode(data)
            if not msg or 'peer_id' not in msg:
                return
                
            peer_id = msg['peer_id']
            if peer_id == self.peer_id:
                return
                
            # Update peer info in PeerManager
            self.peer_manager.add_or_update_peer(
                peer_id,
                "mdns",
                ip=addr[0],
                port=addr[1],
                socket=client_socket
            )
            
            # Handle messages
            while self.running:
                try:
                    data = client_socket.recv(4096)
                    if not data:
                        break
                        
                    msg = self.message_format.decode(data)
                    if msg and 'content' in msg:
                        self.peer_manager.add_message(peer_id, msg['content'], "mdns", outgoing=False)
                        self.on_message(peer_id, msg['content'], "mdns")
                        
                except socket.timeout:
                    # Send keepalive
                    keepalive = self.message_format.encode({'type': 'keepalive'})
                    client_socket.sendall(keepalive)
                    
        except Exception as e:
            self.log(f"Client handler error: {str(e)}")
        finally:
            client_socket.close()
            if peer_id:
                self.peer_manager.mark_peer_inactive(peer_id, "mdns")
                
    def _send_message_impl(self, peer_id: str, message: str) -> bool:
        """Implement message sending"""
        peer = self.peer_manager.get_peer(peer_id)
        if not peer or not peer.is_active("mdns"):
            return False
            
        peer_info = peer.get_protocol_info("mdns")
        
        try:
            # Use existing connection or create new one
            socket_obj = peer_info.get("socket")
            if not socket_obj:
                socket_obj = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                socket_obj.connect((peer_info["ip"], peer_info["port"]))
                
                # Send identification
                ident = self.message_format.encode({
                    'peer_id': self.peer_id,
                    'type': 'ident'
                })
                socket_obj.sendall(ident)
                
                # Update peer with socket
                self.peer_manager.add_or_update_peer(
                    peer_id,
                    "mdns",
                    socket=socket_obj
                )
                
                # Start handler thread
                handler = threading.Thread(
                    target=self._handle_client,
                    args=(socket_obj, (peer_info["ip"], peer_info["port"]))
                )
                handler.daemon = True
                handler.start()
                
            # Send message
            msg = self.message_format.encode({
                'peer_id': self.peer_id,
                'type': 'message',
                'content': message
            })
            socket_obj.sendall(msg)
            
            # Track message in PeerManager
            self.peer_manager.add_message(peer_id, message, "mdns", outgoing=True)
            return True
            
        except Exception as e:
            self.log(f"Error sending message: {str(e)}")
            self.peer_manager.mark_peer_inactive(peer_id, "mdns")
            return False
            
    def _cleanup(self):
        """Clean up resources"""
        try:
            if self.zeroconf and self.service_info:
                self.zeroconf.unregister_service(self.service_info)
                self.zeroconf.close()
                
            if self.server_socket:
                self.server_socket.close()
                
            for sock in self.client_sockets.values():
                sock.close()
            self.client_sockets.clear()
        except Exception as e:
            self.log(f"Error during cleanup: {str(e)}")
            
    def _get_local_ip(self) -> str:
        """Get local IP address"""
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            s.connect(('8.8.8.8', 80))
            return s.getsockname()[0]
        except Exception:
            return '127.0.0.1'
        finally:
            s.close()