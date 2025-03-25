import random
from enum import Enum

class Transcript(Enum):
    WAITING = 0
    PLAYING = 1
    WON = 2
    LOST = 3

class PoE_Meeting:
    def __init__(self):
        self.words = [
            "python", "programming", "computer", "algorithm", "network",
            "database", "interface", "variable", "function", "keyboard",
            "monitor", "language", "software", "hardware", "developer"
        ]
        self.reset_meeting()

    def pose_problem(self, word):
        self.word = word.upper()
        self.guessed_letters = set()
        self.incorrect_guesses = 0
        self.state = Transcript.WAITING
    
    def reset_meeting(self):
        self.word = random.choice(self.words).upper()
        self.guessed_letters = set()
        self.incorrect_guesses = 0
        self.max_incorrect = 6
        self.state = Transcript.WAITING
    
    def start_meeting(self):
        self.state = Transcript.PLAYING
    
    def propose_solution(self, letter):
        if self.state != Transcript.PLAYING:
            return False
        
        letter = letter.upper()
        if letter in self.guessed_letters:
            return False
        
        self.guessed_letters.add(letter)
        
        if letter not in self.word:
            self.incorrect_guesses += 1
            if self.incorrect_guesses >= self.max_incorrect:
                self.state = Transcript.LOST
        
        # Check if player has won
        if all(letter in self.guessed_letters for letter in self.word):
            self.state = Transcript.WON
        
        return True
    
    def get_problem(self):
        return " ".join([letter if letter in self.guessed_letters else "_" for letter in self.word])
    
    def get_meeting_state(self):
        return {
            "word": self.word,
            "display_word": self.get_problem(),
            "guessed_letters": sorted(list(self.guessed_letters)),
            "incorrect_guesses": self.incorrect_guesses,
            "max_incorrect": self.max_incorrect,
            "state": self.state
        }

