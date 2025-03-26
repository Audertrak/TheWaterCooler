import socket
import json
import time
from typing import Callable, Dict, Tuple, Optional

from .base import ProtocolBase

from message.base import MessageBase
from message.json import JSONMessage
from message.protobuf import SimpleProtobufMessage
from message.mqtt import MQTTMessage
from message.raw import RawMessage

class UDPProtocol(ProtocolBase):
    def __init__(self, peer_id: str, on_peer_discovered: Callable, on_message: Callable,
             port: int = 5555, broadcast_interval: int = 5, 
             message_format: Optional[MessageBase] = None):
        super().__init__(peer_id, on_peer_discovered, on_message)
        self.port = port
        self.broadcast_interval = broadcast_interval
        self.socket = None
        self.last_broadcast_time = 0
    
        # Use the provided message format or default to JSON
        self.message_format = message_format or JSONMessage() 
        
    def _run(self):
        """Main UDP listener and broadcaster loop"""
        try:
            # Create UDP socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.socket.bind(('', self.port))
            self.socket.settimeout(1.0)  # 1 second timeout for socket operations
            
            self.log(f"UDP listening on port {self.port}")
            
            while self.running:
                # Broadcast presence periodically
                current_time = time.time()
                if current_time - self.last_broadcast_time > self.broadcast_interval:
                    self._broadcast_presence()
                    self.last_broadcast_time = current_time
                
                # Listen for incoming messages
                try:
                    data, addr = self.socket.recvfrom(4096)
                    self._handle_message(data, addr)
                except socket.timeout:
                    pass  # Expected timeout, continue loop
                
        except Exception as e:
            self.log(f"UDP error: {str(e)}")
        finally:
            if self.socket:
                self.socket.close()
                self.socket = None
    
    def _broadcast_presence(self):
        """Broadcast peer presence to the local network"""
        if not self.socket:
            return
        
        message = self.message_format.create_discovery_message(self.peer_id, "udp")
        
        try:
            data = self.message_format.serialize(message)
            self.socket.sendto(data, ('<broadcast>', self.port))
            self.log(f"Broadcasted presence: {self.peer_id}")
        except Exception as e:
            self.log(f"Broadcast error: {str(e)}")
    
    def _handle_message(self, data: bytes, addr: Tuple[str, int]):
        """Handle incoming UDP message"""
        try:
            message = self.message_format.deserialize(data)
            sender_ip = addr[0]
            
            # Extract message fields
            if isinstance(message, dict):
                message_type = message.get("type")
                peer_id = message.get("peer_id")
                content = self.message_format.extract_content(message)
                
                # Don't process our own messages
                if peer_id == self.peer_id:
                    return
                    
                if message_type == "discovery":
                    # Handle peer discovery
                    if peer_id not in self.peers:
                        self.peers[peer_id] = {"ip": sender_ip, "port": self.port}
                        self.log(f"Discovered peer: {peer_id} at {sender_ip}")
                        self.on_peer_discovered(peer_id, "udp")
                        
                        # Send a response to acknowledge
                        self._send_discovery_response(sender_ip)
                        
                elif message_type == "discovery_response":
                    # Handle discovery response
                    if peer_id not in self.peers:
                        self.peers[peer_id] = {"ip": sender_ip, "port": self.port}
                        self.log(f"Received discovery response from: {peer_id}")
                        self.on_peer_discovered(peer_id, "udp")
                        
                elif message_type == "message":
                    # Handle normal message
                    self.log(f"Received message from {peer_id}: {content}")
                    self.on_message(peer_id, content, "udp")
                    
        except Exception as e:
            self.log(f"Error handling message: {str(e)}")
        
    def _send_discovery_response(self, target_ip: str):
        """Send a discovery response to a specific IP"""
        if not self.socket:
            return
            
        message = self.message_format.create_discovery_response(
            self.peer_id, "udp"
        )
        
        try:
            data = self.message_format.serialize(message)
            self.socket.sendto(data, (target_ip, self.port))
        except Exception as e:
            self.log(f"Error sending discovery response: {str(e)}")

    def _send_message_impl(self, peer_id: str, message: str) -> bool:
        """Send a message to a specific peer"""
        if not self.socket or peer_id not in self.peers:
            return False
            
        peer_info = self.peers[peer_id]
        msg_obj = self.message_format.create_message(
            self.peer_id,
            message,
            "message",
            "udp"
        )
    
        try:
            data = self.message_format.serialize(msg_obj)
            self.socket.sendto(data, (peer_info["ip"], peer_info["port"]))
            self.log(f"Sent message to {peer_id}")
            return True
        except Exception as e:
            self.log(f"Error sending message: {str(e)}")
            return False