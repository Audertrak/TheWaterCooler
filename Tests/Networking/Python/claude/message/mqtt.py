import json
import time
from typing import Any, Dict, Optional, Union, Tuple
from .base import MessageBase


class MQTTMessage(MessageBase):
    """
    MQTT-style message format with topics
    
    While not actually using the MQTT protocol itself (which would require a broker),
    this message format follows MQTT patterns with topics and payloads.
    """
    
    def __init__(self):
        super().__init__()
        self.content_type = "application/mqtt"
    
    def serialize(self, data: Any) -> bytes:
        """Convert message to bytes for transmission"""
        if isinstance(data, dict) and "topic" in data and "payload" in data:
            # It's already in our topic/payload format
            topic = data["topic"]
            payload = data["payload"]
            
            # Convert to our wire format: [topic_length][topic][payload]
            topic_bytes = topic.encode('utf-8')
            topic_len = len(topic_bytes)
            
            # Convert payload to JSON if it's not already a string
            if not isinstance(payload, (str, bytes)):
                payload = json.dumps(payload)
                
            if isinstance(payload, str):
                payload_bytes = payload.encode('utf-8')
            else:
                payload_bytes = payload
                
            # Create the full message
            result = bytearray()
            result.extend(topic_len.to_bytes(4, byteorder='big'))
            result.extend(topic_bytes)
            result.extend(payload_bytes)
            
            return bytes(result)
        
        elif isinstance(data, dict):
            # Convert a regular message dict to topic/payload format
            msg_type = data.get("type", "message")
            peer_id = data.get("peer_id", "unknown")
            protocol = data.get("protocol", "mqtt")
            
            # Build topic based on message type
            if msg_type == "discovery" or msg_type == "discovery_response":
                topic = f"p2p/discovery/{protocol}"
            else:
                topic = f"p2p/messages/{peer_id}"
                
            # Convert the whole dict to JSON for the payload
            payload = json.dumps(data)
            
            # Serialize using our helper method
            topic_bytes = topic.encode('utf-8')
            topic_len = len(topic_bytes)
            payload_bytes = payload.encode('utf-8')
            
            result = bytearray()
            result.extend(topic_len.to_bytes(4, byteorder='big'))
            result.extend(topic_bytes)
            result.extend(payload_bytes)
            
            return bytes(result)
        
        else:
            # For simple data, use a default topic
            topic = "p2p/messages/default"
            payload = str(data)
            
            topic_bytes = topic.encode('utf-8')
            topic_len = len(topic_bytes)
            payload_bytes = payload.encode('utf-8')
            
            result = bytearray()
            result.extend(topic_len.to_bytes(4, byteorder='big'))
            result.extend(topic_bytes)
            result.extend(payload_bytes)
            
            return bytes(result)
    
    def deserialize(self, data: bytes) -> Any:
        """Convert received bytes to topic/payload structure"""
        try:
            # Extract topic length (first 4 bytes)
            if len(data) < 4:
                raise ValueError("Data too short")
                
            topic_len = int.from_bytes(data[0:4], byteorder='big')
            
            # Extract topic and payload
            topic_end = 4 + topic_len
            if len(data) < topic_end:
                raise ValueError("Data too short for topic")
                
            topic = data[4:topic_end].decode('utf-8')
            payload = data[topic_end:]
            
            # Try to parse payload as JSON
            try:
                payload_dict = json.loads(payload)
                return {
                    "topic": topic,
                    "payload": payload_dict
                }
            except:
                # If not JSON, return as string
                return {
                    "topic": topic,
                    "payload": payload.decode('utf-8', errors='replace')
                }
                
        except Exception as e:
            # If parsing fails, return original data
            return {
                "topic": "p2p/error",
                "payload": str(data),
                "error": str(e)
            }
    
    def create_message(self, peer_id: str, content: Any, message_type: str = "message",
                      protocol: Optional[str] = None, **kwargs) -> Dict:
        """Create a structured message with MQTT topic"""
        protocol = protocol or "mqtt"
        
        # Determine the appropriate topic
        if message_type == "discovery" or message_type == "discovery_response":
            topic = f"p2p/discovery/{protocol}"
        else:
            topic = f"p2p/messages/{peer_id}"
        
        # Create the payload
        payload = {
            "type": message_type,
            "peer_id": peer_id,
            "timestamp": int(time.time()),
            "protocol": protocol
        }
        
        # Only include content if it's not None
        if content is not None:
            payload["content"] = content
            
        # Add any additional parameters
        payload.update(kwargs)
        
        # Return the topic/payload structure
        return {
            "topic": topic,
            "payload": payload
        }
    
    def extract_content(self, message: Dict) -> Any:
        """Extract the content from an MQTT message"""
        if isinstance(message, dict):
            if "payload" in message and isinstance(message["payload"], dict):
                # It's in MQTT format
                return message["payload"].get("content")
            elif "content" in message:
                # It's in our regular message format
                return message["content"]
        
        # Otherwise return the whole message
        return message
