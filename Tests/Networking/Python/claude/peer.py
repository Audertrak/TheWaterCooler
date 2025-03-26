import time
import uuid
import socket
import json
from pathlib import Path
from typing import Dict, List, Optional, Set, Any



class Peer:
    """
    Represents a remote peer in the P2P network.
    Tracks connection information and available protocols.
    """
    
    def __init__(self, peer_id: str, protocol: str, **kwargs):
        self.peer_id = peer_id
        self.first_seen = time.time()
        self.last_seen = self.first_seen
        
        # Protocol-specific connection information
        self.protocols = {protocol: kwargs}
        
        # Track which protocols are currently active
        self.active_protocols = set([protocol])
        
        # Track message history with this peer
        self.message_history = []
        
    def update(self, protocol: str, **kwargs):
        """Update peer information for a specific protocol"""
        self.last_seen = time.time()
        
        # Update protocol-specific info
        if protocol in self.protocols:
            self.protocols[protocol].update(kwargs)
        else:
            self.protocols[protocol] = kwargs
            
        # Mark this protocol as active
        self.active_protocols.add(protocol)
        
    def add_message(self, message: str, protocol: str, outgoing: bool = False):
        """Add a message to the history with this peer"""
        self.message_history.append({
            "timestamp": time.time(),
            "message": message,
            "protocol": protocol,
            "outgoing": outgoing
        })
        
        # Keep only the last 100 messages
        if len(self.message_history) > 100:
            self.message_history = self.message_history[-100:]
        
    def is_active(self, protocol: Optional[str] = None) -> bool:
        """Check if the peer is active on a specific protocol or any protocol"""
        if protocol:
            return protocol in self.active_protocols
        else:
            return len(self.active_protocols) > 0
            
    def mark_inactive(self, protocol: str):
        """Mark a protocol as inactive for this peer"""
        if protocol in self.active_protocols:
            self.active_protocols.remove(protocol)
            
    def get_protocol_info(self, protocol: str) -> Dict:
        """Get connection information for a specific protocol"""
        return self.protocols.get(protocol, {})
    
    def get_protocols(self) -> Set[str]:
        """Get all protocols this peer is associated with"""
        return set(self.protocols.keys())
    
    def get_active_protocols(self) -> Set[str]:
        """Get all active protocols for this peer"""
        return self.active_protocols
    
    def get_last_seen_time(self) -> float:
        """Get the last time this peer was seen/updated"""
        return self.last_seen
    
    def time_since_last_seen(self) -> float:
        """Get the number of seconds since this peer was last seen"""
        return time.time() - self.last_seen
    
    def __str__(self) -> str:
        """String representation of the peer"""
        protocol_str = ", ".join(self.active_protocols)
        return f"{self.peer_id} [{protocol_str}]"


class PeerManager:
    """
    Manages all known peers across different protocols.
    Provides central tracking and status updates.
    """
    
    def __init__(self):
        self.peers: Dict[str, Peer] = {}
        self.callbacks = []
        
    def add_or_update_peer(self, peer_id: str, protocol: str, **kwargs) -> Peer:
        """Add a new peer or update an existing one"""
        if peer_id in self.peers:
            # Update existing peer
            self.peers[peer_id].update(protocol, **kwargs)
        else:
            # Create new peer
            self.peers[peer_id] = Peer(peer_id, protocol, **kwargs)
            
            # Notify callbacks about new peer
            for callback in self.callbacks:
                callback("new_peer", peer_id, protocol)
                
        return self.peers[peer_id]
    
    def get_peer(self, peer_id: str) -> Optional[Peer]:
        """Get a specific peer by ID"""
        return self.peers.get(peer_id)
    
    def get_all_peers(self) -> Dict[str, Peer]:
        """Get all known peers"""
        return self.peers
    
    def get_active_peers(self, protocol: Optional[str] = None) -> Dict[str, Peer]:
        """Get all active peers, optionally filtered by protocol"""
        if protocol:
            return {pid: peer for pid, peer in self.peers.items() 
                   if peer.is_active(protocol)}
        else:
            return {pid: peer for pid, peer in self.peers.items() 
                   if peer.is_active()}
    
    def mark_peer_inactive(self, peer_id: str, protocol: str):
        """Mark a peer as inactive for a specific protocol"""
        if peer_id in self.peers:
            self.peers[peer_id].mark_inactive(protocol)
            
            # Notify callbacks about peer status change
            for callback in self.callbacks:
                callback("peer_inactive", peer_id, protocol)
    
    def add_message(self, peer_id: str, message: str, protocol: str, outgoing: bool = False):
        """Add a message to a peer's history"""
        if peer_id in self.peers:
            self.peers[peer_id].add_message(message, protocol, outgoing)
            
            # Update last seen time for inbound messages
            if not outgoing:
                self.peers[peer_id].last_seen = time.time()
    
    def cleanup_inactive_peers(self, timeout: float = 300.0):
        """Remove peers that haven't been seen for a while"""
        current_time = time.time()
        to_remove = []
        
        for peer_id, peer in self.peers.items():
            if current_time - peer.last_seen > timeout:
                to_remove.append(peer_id)
                
        for peer_id in to_remove:
            del self.peers[peer_id]
            
            # Notify callbacks about peer removal
            for callback in self.callbacks:
                callback("peer_removed", peer_id, None)
    
    def register_callback(self, callback):
        """Register a callback for peer events"""
        self.callbacks.append(callback)
    
    def unregister_callback(self, callback):
        """Unregister a callback"""
        if callback in self.callbacks:
            self.callbacks.remove(callback)
            
    def generate_peer_id(self) -> str:
        """Generate a unique peer ID"""
        return f"peer-{uuid.uuid4().hex[:8]}"

class PeerIdentity:
    def __init__(self, user_id: str = "", display_name: str = ""):
        self.peer_id = f"peer-{uuid.uuid4().hex[:8]}"
        self.user_id = user_id or f"user-{uuid.uuid4().hex[:8]}"
        self.display_name = display_name or socket.gethostname()
        self.hostname = socket.gethostname()
        self.domain = socket.getfqdn()
        self.public_key = uuid.uuid4().hex
        self.windows_username = ""
        self.windows_domain = ""
        self.windows_computer = ""

        try:
            import win32api
            import win32con
            self.windows_username = win32api.GetUserNameEx(win32con.NameSamCompatible)
            self.windows_domain = win32api.GetDomainName()
            self.windows_computer = win32api.GetComputerName()
        except ImportError:
            pass
        
    def to_dict(self) -> Dict:
        return {
            "peer_id": self.peer_id,
            "user_id": self.user_id,
            "display_name": self.display_name,
            "hostname": self.hostname,
            "domain": self.domain,
            "public_key": self.public_key,
            "windows_username": self.windows_username,
            "windows_domain": self.windows_domain,
            "windows_computer": self.windows_computer
        }

    def to_sharing_string(self) -> str:
        """Generate a shareable string format of identity"""
        data = self.to_dict()
        return f"p2p://{json.dumps(data)}"
    
    @classmethod
    def from_sharing_string(cls, sharing_string: str) -> Optional['PeerIdentity']:
        """Create identity from sharing string"""
        try:
            if sharing_string.startswith("p2p://"):
                data = json.loads(sharing_string[6:])
                identity = cls()
                identity.peer_id = data["peer_id"]
                identity.user_id = data["user_id"]
                identity.display_name = data["display_name"]
                identity.hostname = data["hostname"]
                identity.domain = data["domain"]
                identity.public_key = data["public_key"]
                return identity
        except:
            return None
        return None
    
    def save_to_file(self, filepath: Path):
        """Save identity to file"""
        with open(filepath, 'w') as f:
            json.dump(self.to_dict(), f, indent=2)
    
    @classmethod
    def load_from_file(cls, filepath: Path) -> Optional['PeerIdentity']:
        """Load identity from file"""
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
                identity = cls()
                identity.peer_id = data["peer_id"]
                identity.user_id = data["user_id"]
                identity.display_name = data["display_name"]
                identity.hostname = data["hostname"]
                identity.domain = data["domain"]
                identity.public_key = data["public_key"]
                return identity
        except:
            return None