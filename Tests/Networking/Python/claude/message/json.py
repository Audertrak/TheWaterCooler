import json
import time
from typing import Any, Dict, Optional, Union
from .base import MessageBase


class JSONMessage(MessageBase):
    """JSON-based message format"""
    
    def __init__(self, pretty_print: bool = False):
        super().__init__()
        self.content_type = "application/json"
        self.pretty_print = pretty_print
    
    def serialize(self, data: Any) -> bytes:
        """Convert Python data to JSON bytes"""
        if self.pretty_print:
            json_str = json.dumps(data, indent=2)
        else:
            json_str = json.dumps(data)
        return json_str.encode('utf-8')
    
    def deserialize(self, data: bytes) -> Any:
        """Convert JSON bytes to Python objects"""
        try:
            return json.loads(data.decode('utf-8'))
        except json.JSONDecodeError:
            # If not valid JSON, return as string
            return data.decode('utf-8', errors='replace')
        except Exception:
            # If decoding fails, return raw bytes
            return data
    
    def create_message(self, peer_id: str, content: Any, message_type: str = "message",
                      protocol: Optional[str] = None, **kwargs) -> Dict:
        """Create a structured JSON message"""
        message = {
            "type": message_type,
            "peer_id": peer_id,
            "timestamp": int(time.time()),
            "protocol": protocol or "json"
        }
        
        # Only include content if it's not None
        if content is not None:
            message["content"] = content
        
        # Add any additional parameters
        message.update(kwargs)
        
        return message
    
    def extract_content(self, message: Dict) -> Any:
        """Extract the content from a JSON message"""
        if isinstance(message, dict) and "content" in message:
            return message["content"]
        else:
            # If it's not a dict or doesn't have content, return as is
            return message
