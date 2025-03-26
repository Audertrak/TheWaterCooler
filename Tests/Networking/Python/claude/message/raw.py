from typing import Any, Dict, Optional, Union
import time
from .base import MessageBase


class RawMessage(MessageBase):
    """Simple string-based message format"""
    
    def __init__(self):
        super().__init__()
        self.content_type = "text/plain"
    
    def serialize(self, data: Any) -> bytes:
        """Convert data to bytes for transmission"""
        if isinstance(data, dict):
            # If it's a structured message, convert to a simple format
            # Format: type|peer_id|timestamp|protocol|content
            msg_type = data.get("type", "message")
            peer_id = data.get("peer_id", "unknown")
            timestamp = data.get("timestamp", str(int(time.time())))
            protocol = data.get("protocol", "unknown")
            content = data.get("content", "")
            
            formatted = f"{msg_type}|{peer_id}|{timestamp}|{protocol}|{content}"
            return formatted.encode('utf-8')
        elif isinstance(data, str):
            # If it's just a string, encode it directly
            return data.encode('utf-8')
        else:
            # For any other type, convert to string first
            return str(data).encode('utf-8')
    
    def deserialize(self, data: bytes) -> Any:
        """Convert received bytes to Python objects"""
        try:
            # Try to parse as structured message
            text = data.decode('utf-8')
            parts = text.split('|', 4)  # Max 5 parts
            
            if len(parts) == 5:
                return {
                    "type": parts[0],
                    "peer_id": parts[1],
                    "timestamp": parts[2],
                    "protocol": parts[3],
                    "content": parts[4]
                }
            else:
                # Not structured, just return the text
                return text
        except Exception:
            # If decoding fails, return raw bytes
            return data
    
    def create_message(self, peer_id: str, content: Any, message_type: str = "message",
                      protocol: Optional[str] = None, **kwargs) -> Dict:
        """Create a structured message"""
        message = {
            "type": message_type,
            "peer_id": peer_id,
            "timestamp": str(int(time.time())),
            "protocol": protocol or "raw",
            "content": str(content) if content is not None else ""
        }
        
        # Add any additional parameters
        message.update(kwargs)
        
        return message
    
    def extract_content(self, message: Dict) -> Any:
        """Extract the content from a structured message"""
        if isinstance(message, dict) and "content" in message:
            return message["content"]
        else:
            # If it's not a dict or doesn't have content, return as is
            return message
