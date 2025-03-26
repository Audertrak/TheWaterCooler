from typing import Any, Dict, Optional, Union
import time
import struct
from .base import MessageBase


class SimpleProtobufMessage(MessageBase):
    """
    A simplified "proto-like" binary message format
    
    This is not a true Protocol Buffers implementation (which would require the protobuf library),
    but instead a simple binary format inspired by it.
    
    Message format:
    - 1 byte: Version (0x01)
    - 1 byte: Message type (0=message, 1=discovery, 2=discovery_response)
    - 4 bytes: Length of peer_id string
    - N bytes: peer_id as UTF-8
    - 8 bytes: Timestamp (seconds since epoch) as uint64
    - 4 bytes: Length of protocol string
    - N bytes: protocol as UTF-8
    - 4 bytes: Length of content
    - N bytes: content as UTF-8
    """
    
    # Message type constants
    TYPE_MESSAGE = 0
    TYPE_DISCOVERY = 1
    TYPE_DISCOVERY_RESPONSE = 2
    
    def __init__(self):
        super().__init__()
        self.content_type = "application/x-protobuf"
        self.version = 1
    
    def serialize(self, data: Any) -> bytes:
        """Convert message dict to binary format"""
        if not isinstance(data, dict):
            # If not a dict, wrap it in a message dict
            data = {
                "type": "message",
                "peer_id": "unknown",
                "timestamp": int(time.time()),
                "protocol": "protobuf",
                "content": str(data)
            }
        
        # Extract message components
        msg_type_str = data.get("type", "message")
        if msg_type_str == "message":
            msg_type = self.TYPE_MESSAGE
        elif msg_type_str == "discovery":
            msg_type = self.TYPE_DISCOVERY
        elif msg_type_str == "discovery_response":
            msg_type = self.TYPE_DISCOVERY_RESPONSE
        else:
            msg_type = self.TYPE_MESSAGE
        
        peer_id = data.get("peer_id", "unknown").encode('utf-8')
        timestamp = int(data.get("timestamp", time.time()))
        protocol = data.get("protocol", "protobuf").encode('utf-8')
        
        content = ""
        if "content" in data:
            content = str(data["content"])
        content_bytes = content.encode('utf-8')
        
        # Construct binary message
        result = bytearray()
        result.append(self.version)  # Version
        result.append(msg_type)      # Message type
        
        # Peer ID
        result.extend(struct.pack("!I", len(peer_id)))  # Length as 4-byte unsigned int
        result.extend(peer_id)
        
        # Timestamp (8 bytes)
        result.extend(struct.pack("!Q", timestamp))
        
        # Protocol
        result.extend(struct.pack("!I", len(protocol)))
        result.extend(protocol)
        
        # Content
        result.extend(struct.pack("!I", len(content_bytes)))
        result.extend(content_bytes)
        
        return bytes(result)
    
    def deserialize(self, data: bytes) -> Any:
        """Convert binary format back to message dict"""
        try:
            if len(data) < 10:  # Minimum header size
                raise ValueError("Data too short")
            
            offset = 0
            
            # Version and message type
            version = data[offset]
            offset += 1
            msg_type = data[offset]
            offset += 1
            
            if version != self.version:
                raise ValueError(f"Unsupported version: {version}")
            
            # Convert numeric message type to string
            if msg_type == self.TYPE_MESSAGE:
                type_str = "message"
            elif msg_type == self.TYPE_DISCOVERY:
                type_str = "discovery"
            elif msg_type == self.TYPE_DISCOVERY_RESPONSE:
                type_str = "discovery_response"
            else:
                type_str = f"unknown_{msg_type}"
            
            # Peer ID
            peer_id_len = struct.unpack("!I", data[offset:offset+4])[0]
            offset += 4
            peer_id = data[offset:offset+peer_id_len].decode('utf-8')
            offset += peer_id_len
            
            # Timestamp
            timestamp = struct.unpack("!Q", data[offset:offset+8])[0]
            offset += 8
            
            # Protocol
            protocol_len = struct.unpack("!I", data[offset:offset+4])[0]
            offset += 4
            protocol = data[offset:offset+protocol_len].decode('utf-8')
            offset += protocol_len
            
            # Content
            content_len = struct.unpack("!I", data[offset:offset+4])[0]
            offset += 4
            content = data[offset:offset+content_len].decode('utf-8')
            
            # Construct message dict
            message = {
                "type": type_str,
                "peer_id": peer_id,
                "timestamp": timestamp,
                "protocol": protocol
            }
            
            # Only include content if it's not empty
            if content:
                message["content"] = content
                
            return message
            
        except Exception as e:
            # If parsing fails, return raw bytes
            return {
                "type": "unknown",
                "peer_id": "unknown",
                "timestamp": int(time.time()),
                "protocol": "protobuf",
                "content": str(data),
                "error": str(e)
            }
    
    def create_message(self, peer_id: str, content: Any, message_type: str = "message",
                      protocol: Optional[str] = None, **kwargs) -> Dict:
        """Create a structured message"""
        message = {
            "type": message_type,
            "peer_id": peer_id,
            "timestamp": int(time.time()),
            "protocol": protocol or "protobuf"
        }
        
        # Only include content if it's not None
        if content is not None:
            message["content"] = content
        
        # Add any additional parameters
        message.update(kwargs)
        
        return message
    
    def extract_content(self, message: Dict) -> Any:
        """Extract the content from a message"""
        if isinstance(message, dict) and "content" in message:
            return message["content"]
        else:
            return message
