import socket
import threading
import json
import time

class HangmanServer:
    def __init__(self, game, ui, host='0.0.0.0', port=5555):
        self.game = game
        self.ui = ui
        self.host = host
        self.port = port
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.clients = []
        self.running = False
    
    def start(self):
        try:
            self.server.bind((self.host, self.port))
            self.server.listen(5)
            self.running = True
            
            # Get local IP address for display
            hostname = socket.gethostname()
            local_ip = socket.gethostbyname(hostname)
            print(f"Server started on {local_ip}:{self.port}")
            
            # Start listening for connections in a separate thread
            thread = threading.Thread(target=self.accept_connections)
            thread.daemon = True
            thread.start()
            
            return True
        except Exception as e:
            print(f"Error starting server: {e}")
            return False
    
    def accept_connections(self):
        while self.running:
            try:
                client, address = self.server.accept()
                print(f"Connection from {address}")
                
                # Add client to list
                self.clients.append(client)
                
                # Start thread to handle client
                thread = threading.Thread(target=self.handle_client, args=(client,))
                thread.daemon = True
                thread.start()
                
                # Send current game state to new client
                self.send_game_state(client)
            except Exception as e:
                if self.running:
                    print(f"Error accepting connection: {e}")
                    time.sleep(1)
    
    def handle_client(self, client):
        while self.running:
            try:
                data = client.recv(1024).decode('utf-8')
                if not data:
                    break
                
                message = json.loads(data)
                self.process_message(message)
            except Exception as e:
                print(f"Error handling client: {e}")
                break
        
        # Remove disconnected client
        if client in self.clients:
            self.clients.remove(client)
    
    def process_message(self, message):
        if message["type"] == "guess":
            letter = message["letter"]
            self.game.guess_letter(letter)
            
            # Update UI and broadcast new state
            self.ui.update_display()
            self.broadcast_game_state()
    
    def send_game_state(self, client=None):
        game_state = self.game.get_game_state()
        message = {
            "type": "game_state",
            "data": {
                "actual_word": game_state["word"],
                "display_word": game_state["display_word"],
                "guessed_letters": game_state["guessed_letters"],
                "incorrect_guesses": game_state["incorrect_guesses"],
                "max_incorrect": game_state["max_incorrect"],
                "state": game_state["state"].value
            }
        }
        
        data = json.dumps(message).encode('utf-8')
        
        if client:
            try:
                client.send(data)
            except Exception as e:
                print(f"Error sending to client: {e}")
                if client in self.clients:
                    self.clients.remove(client)
        else:
            # Broadcast to all clients
            self.broadcast(data)
    
    def broadcast_game_state(self):
        self.send_game_state()
    
    def broadcast(self, message):
        disconnected_clients = []
        
        for client in self.clients:
            try:
                client.send(message)
            except Exception as e:
                print(f"Error broadcasting to client: {e}")
                disconnected_clients.append(client)
        
        # Remove disconnected clients
        for client in disconnected_clients:
            if client in self.clients:
                self.clients.remove(client)
    
    def start_new_game(self):
        self.game.reset_game()
        self.game.start_game()
        self.ui.update_display()
        self.broadcast_game_state()
    
    def stop(self):
        self.running = False
        for client in self.clients:
            try:
                client.close()
            except:
                pass
        self.server.close()

class HangmanClient:
    def __init__(self, game, ui, host, port=5555):
        self.game = game
        self.ui = ui
        self.host = host
        self.port = port
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.running = False
    
    def connect(self):
        try:
            self.client.connect((self.host, self.port))
            self.running = True
            
            # Start thread to receive messages
            thread = threading.Thread(target=self.receive_messages)
            thread.daemon = True
            thread.start()
            
            return True
        except Exception as e:
            print(f"Error connecting to server: {e}")
            return False
    
    def receive_messages(self):
        while self.running:
            try:
                data = self.client.recv(1024).decode('utf-8')
                if not data:
                    break
                
                message = json.loads(data)
                self.process_message(message)
            except Exception as e:
                if self.running:
                    print(f"Error receiving message: {e}")
                    break
        
        self.disconnect()
    
    def process_message(self, message):
        if message["type"] == "game_state":
            data = message["data"]
            
            # Update game state based on server data
            from logic import GameState
            
            # Update guessed letters
            self.game.word = data["actual_word"]
            self.game.guessed_letters = set(data["guessed_letters"])
            self.game.incorrect_guesses = data["incorrect_guesses"]
            self.game.state = GameState(data["state"])
            
            # Update UI
            self.ui.root.after(0, self.ui.update_display)
    
    def send_guess(self, letter):
        message = {
            "type": "guess",
            "letter": letter
        }
        
        self.send_message(message)
    
    def send_message(self, message):
        try:
            data = json.dumps(message).encode('utf-8')
            self.client.send(data)
        except Exception as e:
            print(f"Error sending message: {e}")
            self.disconnect()
    
    def disconnect(self):
        self.running = False
        try:
            self.client.close()
        except:
            pass

