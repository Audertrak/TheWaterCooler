# utils.py
import socket
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def get_lan_ip():
    """
    Attempts to find the local LAN IP address of the machine.
    Tries connecting to a public DNS server (Google's) to determine the
    interface used for outgoing connections.
    """
    s = None
    try:
        # Connect to an external host to find the preferred outgoing IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Doesn't actually send data, just sets up the connection endpoint
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        return ip
    except socket.error as e:
        logging.error(f"Could not determine LAN IP: {e}")
        # Fallback if external connection fails
        try:
            return socket.gethostbyname(socket.gethostname())
        except socket.gaierror:
            logging.error("Fallback gethostbyname failed. Returning 127.0.0.1")
            return "127.0.0.1" # Last resort
    finally:
        if s:
            s.close()

if __name__ == '__main__':
    # Test the function if run directly
    print(f"Detected LAN IP: {get_lan_ip()}")
