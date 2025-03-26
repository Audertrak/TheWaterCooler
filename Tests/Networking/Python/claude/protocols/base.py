import abc
import threading
import time
from typing import Callable, Dict, List, Optional, Any


class ProtocolBase(abc.ABC):
    def __init__(self, peer_id: str, on_peer_discovered: Callable, on_message: Callable):
        self.peer_id = peer_id
        self.on_peer_discovered = on_peer_discovered
        self.on_message = on_message
        self.running = False
        self.thread = None
        self.peers = {}  # {peer_id: connection_info}
        self.log_messages = []
        
    def start(self):
        """Start the protocol handler in a separate thread"""
        if self.running:
            return
            
        self.running = True
        self.thread = threading.Thread(target=self._run)
        self.thread.daemon = True
        self.thread.start()
        self.log(f"Starting {self.__class__.__name__}")
        
    def stop(self):
        """Stop the protocol handler"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
            self.thread = None
        self.log(f"Stopped {self.__class__.__name__}")
        
    def send_message(self, peer_id: str, message: str) -> bool:
        """Send a message to a specific peer"""
        if peer_id not in self.peers:
            self.log(f"Unknown peer: {peer_id}")
            return False
        return self._send_message_impl(peer_id, message)
        
    def broadcast_message(self, message: str) -> bool:
        """Send a message to all known peers"""
        success = True
        for peer_id in self.peers:
            if not self.send_message(peer_id, message):
                success = False
        return success
        
    def log(self, message: str):
        """Add a log message"""
        timestamp = time.strftime("%H:%M:%S")
        log_entry = f"[{timestamp}] {message}"
        self.log_messages.append(log_entry)
        # Keep only the last 100 messages
        self.log_messages = self.log_messages[-100:]
        
    @abc.abstractmethod
    def _run(self):
        """Main loop for the protocol handler"""
        pass
        
    @abc.abstractmethod
    def _send_message_impl(self, peer_id: str, message: str) -> bool:
        """Implementation for sending a message to a peer"""
        pass
