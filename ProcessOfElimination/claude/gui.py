import tkinter as tk
from tkinter import messagebox, simpledialog
from logic import GameState

class HangmanUI:
    def __init__(self, root, game, is_host=False, network=None):
        self.root = root
        self.game = game
        self.network = network
        self.is_host = is_host
        
        self.root.title("Process of Elimination")
        self.root.geometry("800x600")
        #self.root.resizable(False, False)
        
        self.setup_ui()
        self.update_display()
    
    def setup_ui(self):
        # Game area frame
        self.game_frame = tk.Frame(self.root, padx=20, pady=20)
        self.game_frame.pack(fill=tk.BOTH, expand=True)
        
        # Hangman display
        self.hangman_canvas = tk.Canvas(self.game_frame, width=200, height=250)
        self.hangman_canvas.grid(row=0, column=0, pady=10)
        
        # Word display
        self.word_display = tk.Label(self.game_frame, text="", font=("Arial", 24))
        self.word_display.grid(row=1, column=0, pady=10)
        
        # Guessed letters
        self.guessed_label = tk.Label(self.game_frame, text="Guessed Letters:", font=("Arial", 12))
        self.guessed_label.grid(row=2, column=0, pady=5)
        
        self.guessed_display = tk.Label(self.game_frame, text="", font=("Arial", 12))
        self.guessed_display.grid(row=3, column=0, pady=5)

        # For players (clients), create controls for guessing
        if not self.is_host:
            self.input_frame = tk.Frame(self.game_frame)
            self.input_frame.grid(row=4, column=0, pady=20)
            
            self.letter_label = tk.Label(
                self.input_frame, text="Enter a letter:", font=("Arial", 12)
            )
            self.letter_label.grid(row=0, column=0, padx=5)
            
            self.letter_entry = tk.Entry(self.input_frame, width=5, font=("Arial", 12))
            self.letter_entry.grid(row=0, column=1, padx=5)
            
            self.guess_button = tk.Button(
                self.input_frame, text="Guess", command=self.make_guess
            )
            self.guess_button.grid(row=0, column=2, padx=5)
            
            # Bind Enter key to make_guess
            self.letter_entry.bind("<Return>", lambda event: self.make_guess())
    
        # Status message
        self.status_message = tk.Label(
            self.game_frame, text="", font=("Arial", 12)
        )
        self.status_message.grid(row=5, column=0, pady=10)
        
        # Create buttons for host actions (only for host)
        if self.is_host:
            self.control_frame = tk.Frame(self.game_frame)
            self.control_frame.grid(row=6, column=0, pady=10)
            
            self.start_button = tk.Button(
                self.control_frame, text="Start New Game", command=self.start_new_game
            )
            self.start_button.grid(row=0, column=0, padx=5)

    def draw_hangman(self, incorrect_guesses):
        self.hangman_canvas.delete("all")
        
        # Gallows
        self.hangman_canvas.create_line(40, 200, 160, 200, width=3)  # Base
        self.hangman_canvas.create_line(60, 200, 60, 50, width=3)    # Pole
        self.hangman_canvas.create_line(60, 50, 120, 50, width=3)    # Top
        self.hangman_canvas.create_line(120, 50, 120, 70, width=3)   # Noose
        
        # Draw hangman parts based on incorrect guesses
        if incorrect_guesses >= 1:
            # Head
            self.hangman_canvas.create_oval(100, 70, 140, 110, width=2)
        
        if incorrect_guesses >= 2:
            # Body
            self.hangman_canvas.create_line(120, 110, 120, 150, width=2)
        
        if incorrect_guesses >= 3:
            # Left arm
            self.hangman_canvas.create_line(120, 120, 100, 140, width=2)
        
        if incorrect_guesses >= 4:
            # Right arm
            self.hangman_canvas.create_line(120, 120, 140, 140, width=2)
        
        if incorrect_guesses >= 5:
            # Left leg
            self.hangman_canvas.create_line(120, 150, 100, 180, width=2)
        
        if incorrect_guesses >= 6:
            # Right leg
            self.hangman_canvas.create_line(120, 150, 140, 180, width=2)
    
    def update_display(self):
        game_state = self.game.get_game_state()
        
        # Update hangman drawing
        self.draw_hangman(game_state["incorrect_guesses"])
        
        # Update word display
        self.word_display.config(text=game_state["display_word"])
        
        # Update guessed letters
        guessed_str = ", ".join(game_state["guessed_letters"]) if game_state["guessed_letters"] else "None"
        self.guessed_display.config(text=guessed_str)
        
        # Update status message
        if game_state["state"] == GameState.WAITING:
            self.status_message.config(text="Waiting to start...")
            if not self.is_host and hasattr(self, "letter_entry"):
                self.letter_entry.config(state=tk.DISABLED)
                self.guess_button.config(state=tk.DISABLED)
        elif game_state["state"] == GameState.PLAYING:
            self.status_message.config(text=f"Incorrect guesses: {game_state['incorrect_guesses']}/{game_state['max_incorrect']}")
            if not self.is_host and hasattr(self, "letter_entry"):
                self.letter_entry.config(state=tk.NORMAL)
                self.guess_button.config(state=tk.NORMAL)
        elif game_state["state"] == GameState.WON:
            self.status_message.config(text="You won! The word was: " + game_state["word"])
            if not self.is_host and hasattr(self, "letter_entry"):
                self.letter_entry.config(state=tk.DISABLED)
                self.guess_button.config(state=tk.DISABLED)
            if self.is_host and self.network:
                self.start_button.config(state=tk.NORMAL)
            self.root.after(2000, self.exit_game)
        elif game_state["state"] == GameState.LOST:
            self.status_message.config(text="You lost! The word was: " + game_state["word"])
            if not self.is_host and hasattr(self, "letter_entry"):
                self.letter_entry.config(state=tk.DISABLED)
                self.guess_button.config(state=tk.DISABLED)
            if self.is_host and self.network:
                self.start_button.config(state=tk.NORMAL)
            self.root.after(2000, self.exit_game)
    
    def make_guess(self):
        letter = self.letter_entry.get().strip()
        if len(letter) != 1 or not letter.isalpha():
            messagebox.showwarning("Invalid Input", "Please enter a single letter.")
            self.letter_entry.delete(0, tk.END)
            return
        
        # Clear entry field
        self.letter_entry.delete(0, tk.END)
        
        if self.network:
            # Send guess over network
            self.network.send_guess(letter)
        else:
            # Local gameplay
            self.game.guess_letter(letter)
            self.update_display()
    
    def start_new_game(self):
        if self.network:
            self.network.start_new_game()
        else:
            self.game.reset_game()
            self.game.start_game()
            self.update_display()


    def exit_game(self):
        if self.network:
            self.network.stop()
        self.root.destroy()
