import abc
from typing import Any, Dict, Optional, Union


class MessageBase(abc.ABC):
    """Base class for message format implementations"""
    
    def __init__(self):
        self.content_type = "text/plain"
    
    @abc.abstractmethod
    def serialize(self, data: Any) -> bytes:
        """Convert Python data to serialized format for transmission"""
        pass
    
    @abc.abstractmethod
    def deserialize(self, data: bytes) -> Any:
        """Convert received data from serialized format back to Python objects"""
        pass
    
    @abc.abstractmethod
    def create_message(self, peer_id: str, content: Any, message_type: str = "message",
                      protocol: Optional[str] = None) -> Dict:
        """Create a structured message with appropriate metadata"""
        pass
    
    @abc.abstractmethod
    def extract_content(self, message: Dict) -> Any:
        """Extract the content from a structured message"""
        pass
    
    def get_content_type(self) -> str:
        """Get the MIME content type for this message format"""
        return self.content_type
    
    def create_discovery_message(self, peer_id: str, protocol: str, **kwargs) -> Dict:
        """Create a peer discovery message"""
        return self.create_message(peer_id, None, "discovery", protocol, **kwargs)
    
    def create_discovery_response(self, peer_id: str, protocol: str, **kwargs) -> Dict:
        """Create a response to a discovery message"""
        return self.create_message(peer_id, None, "discovery_response", protocol, **kwargs)
