#TODO: investigate windows-curses and textual as alternatives to msvcrt

import os
import time
import sys
import msvcrt
from logic import Transcript, PoE_Meeting
from network import PoE_Client, PoE_Server

class Application:
    """Application states - matches GUI version for consistency"""
    MAIN_MENU = "main_menu"
    MEETING = "meeting"
    RESULTS = "results"

class Colors:
    """ANSI color codes for Windows console"""
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"
    WHITE = "\033[97m"

class ProcessOfEliminationTUI:
    def __init__(self, meeting=None, is_host=False, network=None):
        """Initialize the TUI application"""
        self.meeting = meeting if meeting else PoE_Meeting()
        self.network = network
        self.is_host = is_host
        
        # Enable ANSI colors in Windows terminal
        os.system("")
        
        # Get terminal dimensions
        self.width = os.get_terminal_size().columns
        self.height = os.get_terminal_size().lines
        
        # Input buffer for user input
        self.input_buffer = ""
        
        # Current state
        self.current_state = None
        
        # Initialize UI
        if self.network and hasattr(self.network, 'running') and self.network.running:
            self.switch_to_state(Application.MEETING)
        else:
            self.switch_to_state(Application.MAIN_MENU)
    
    def clear_screen(self):
        """Clear the console screen"""
        os.system('cls' if os.name == 'nt' else 'clear')
    
    def draw_header(self, title, status=None, status_color=None):
        """Draw the header with title and optional status"""
        header = f"{Colors.BOLD}{title}{Colors.RESET}"
        
        # Add status if provided
        if status:
            color = status_color if status_color else Colors.WHITE
            padding = self.width - len(title) - len(status) - 4
            header += " " * padding + f"{color}{status}{Colors.RESET}"
        
        print(header)
        print("=" * self.width)
    
    def draw_footer(self, message="", input_prompt=None):
        """Draw the footer with message and optional input prompt"""
        print()
        print("=" * self.width)
        
        if input_prompt:
            prompt = f"{input_prompt}: {self.input_buffer}"
            print(prompt, end="", flush=True)
        elif message:
            print(message)
    
    def center_text(self, text, width=None):
        """Center text in the given width"""
        width = width or self.width
        return text.center(width)
    
    def draw_main_menu(self):
        """Draw the main menu screen"""
        self.clear_screen()
        
        # Draw header
        self.draw_header("Process of Elimination")
        
        # Title
        print()
        print(self.center_text(f"{Colors.BLUE}{Colors.BOLD}Process of Elimination{Colors.RESET}"))
        print(self.center_text("A collaborative problem-solving experience"))
        print()
        print()
        
        # Menu options
        menu_items = [
            "1. Host a Meeting",
            "2. Join a Meeting",
            "3. Exit"
        ]
        
        # Display menu items centered
        for item in menu_items:
            print(self.center_text(item))
        
        print()
        print()
        
        # Copyright info
        copyright_text = "© 2025 The Watercooler"
        print(self.center_text(copyright_text))
        
        # Footer
        self.draw_footer("Enter a number to select an option")
    
    def draw_ascii_clock(self, incorrect_guesses, max_incorrect):
        """Draw an ASCII representation of a clock"""
        # Calculate progress
        progress = incorrect_guesses / max_incorrect if max_incorrect > 0 else 0
        
        # Choose color based on urgency
        color = Colors.GREEN
        if progress >= 0.75:
            color = Colors.RED
        elif progress >= 0.5:
            color = Colors.YELLOW
        
        # Calculate time display
        time_left = max_incorrect - incorrect_guesses
        
        # Convert to hours:minutes format for display
        hours = min(11, time_left)  # Cap at 11 for display
        minutes = int((time_left % 1) * 60)  # Convert fractional part to minutes
        
        # Create ASCII clock
        clock = [
            f"{color}+-------------+{Colors.RESET}",
            f"{color}|             |{Colors.RESET}",
            f"{color}|  {Colors.BOLD}{hours:02d}:{minutes:02d}{Colors.RESET}{color}  |{Colors.RESET}",
            f"{color}|             |{Colors.RESET}",
            f"{color}+-------------+{Colors.RESET}",
        ]
        
        # Add time remaining info
        clock.append(f"Time Left: {time_left}/{max_incorrect}")
        
        # Add warning if time is running low
        if progress >= 0.75:
            clock.append(f"{Colors.RED}TIME RUNNING OUT!{Colors.RESET}")
        
        # Print the clock
        for line in clock:
            print(line)
    
    def draw_progress_bar(self, progress, width=40):
        """Draw a progress bar showing completion"""
        filled_width = int(progress * width)
        bar = "[" + "#" * filled_width + " " * (width - filled_width) + "]"
        percent = int(progress * 100)
        return f"Solution Progress: {percent}% {bar}"
    
    def draw_meeting_view(self):
        """Draw the meeting screen with problem and clock"""
        self.clear_screen()
        
        meeting_state = self.meeting.get_meeting_state()
        
        # Determine meeting status and color
        status = "Not Started"
        color = Colors.YELLOW
        
        if meeting_state["state"] == Transcript.PLAYING:
            status = "In Progress"
            color = Colors.GREEN
        elif meeting_state["state"] == Transcript.WON:
            status = "Completed"
            color = Colors.GREEN
        elif meeting_state["state"] == Transcript.LOST:
            status = "Failed"
            color = Colors.RED
        
        # Draw header with status
        self.draw_header("Ongoing Meeting", status, color)
        
        # Draw clock visualization
        self.draw_ascii_clock(meeting_state["incorrect_guesses"], meeting_state["max_incorrect"])
        
        print()
        
        # Display problem (word to guess)
        problem_title = "Current Problem:"
        problem = meeting_state["display_word"]
        
        print(self.center_text(problem_title))
        print(self.center_text(f"{Colors.BOLD}{problem}{Colors.RESET}"))
        
        print()
        
        # Calculate progress for the process diagram
        progress = len([c for c in meeting_state["display_word"] if c.isalpha()]) / len(meeting_state["word"])
        
        # Show progress bar
        print(self.center_text(self.draw_progress_bar(progress)))
        
        print()
        
        # Display proposed solutions
        correct = []
        incorrect = []
        for letter in meeting_state["guessed_letters"]:
            if letter in meeting_state["word"]:
                correct.append(letter)
            else:
                incorrect.append(letter)
        
        correct_str = "Correct Approaches: " + ", ".join(correct) if correct else "Correct Approaches: None"
        incorrect_str = "Incorrect Approaches: " + ", ".join(incorrect) if incorrect else "Incorrect Approaches: None"
        
        print(self.center_text(f"{Colors.GREEN}{correct_str}{Colors.RESET}"))
        print(self.center_text(f"{Colors.RED}{incorrect_str}{Colors.RESET}"))
        
        # Footer with controls or input prompt
        if not self.is_host and meeting_state["state"] == Transcript.PLAYING:
            self.draw_footer(input_prompt="Enter a letter to propose a solution")
        elif self.is_host:
            if meeting_state["state"] == Transcript.WAITING:
                self.draw_footer("Press 'S' to start meeting, 'M' for main menu")
            else:
                self.draw_footer("Press 'M' for main menu")
        else:
            self.draw_footer("Waiting for meeting to start...")
    
    def draw_results_view(self, success, problem):
        """Draw the meeting results screen"""
        self.clear_screen()
        
        # Draw header
        self.draw_header("Meeting Results")
        
        print()
        print(self.center_text(f"{Colors.BLUE}{Colors.BOLD}Meeting Results{Colors.RESET}"))
        print()
        
        # Result message with appropriate color
        if success:
            result_msg = f"{Colors.GREEN}Problem solved successfully!{Colors.RESET}"
        else:
            result_msg = f"{Colors.RED}Time expired before problem was solved.{Colors.RESET}"
        
        print(self.center_text(result_msg))
        print()
        
        # Show the problem
        problem_label = "The problem was:"
        print(self.center_text(problem_label))
        print(self.center_text(f"{Colors.BOLD}{problem}{Colors.RESET}"))
        
        # Footer
        self.draw_footer("Press 'M' to return to main menu")
    
    def switch_to_state(self, state):
        """Switch to a different application state"""
        self.current_state = state
        self.input_buffer = ""
        
        if state == Application.MAIN_MENU:
            self.draw_main_menu()
        elif state == Application.MEETING:
            self.draw_meeting_view()
        elif state == Application.RESULTS:
            meeting_state = self.meeting.get_meeting_state()
            success = meeting_state["state"] == Transcript.WON
            self.draw_results_view(success, meeting_state["word"])
    
    def get_input(self, prompt=None):
        """Get single line input from user"""
        if prompt:
            print(prompt, end="", flush=True)
        
        buffer = ""
        while True:
            if msvcrt.kbhit():
                char = msvcrt.getch().decode('utf-8', errors='ignore')
                
                # Enter key
                if char == '\r':
                    print()  # Move to next line
                    return buffer
                
                # Backspace
                elif char == '\b':
                    if buffer:
                        buffer = buffer[:-1]
                        # Clear the last character on screen
                        print('\b \b', end='', flush=True)
                
                # Regular character
                elif char.isprintable():
                    buffer += char
                    print(char, end='', flush=True)
            
            time.sleep(0.05)  # Small delay to prevent CPU hogging
    
    def host_meeting(self):
        """Host a new meeting"""
        self.clear_screen()
        
        # Draw header
        self.draw_header("Schedule a New Meeting")
        
        # Get problem input
        print()
        problem = self.get_input("Enter a problem for the team to solve: ")
        
        if not problem:
            self.switch_to_state(Application.MAIN_MENU)
            return
        
        self.is_host = True
        
        # Initialize server if needed
        if not self.network:
            try:
                self.network = PoE_Server(self.meeting, self, host='0.0.0.0')
                self.network.start()
                
                # Display server information
                hostname = self.network.host
                port = self.network.port
                print(f"\nServer started at {hostname}:{port}")
                print("Tell your team members to join using this address.")
                print("\nPress any key to continue...")
                msvcrt.getch()
                
            except Exception as e:
                self.show_error(f"Failed to start server: {e}")
                return
        
        # Set the problem
        if hasattr(self.meeting, "pose_problem"):
            self.meeting.pose_problem(problem)
        elif hasattr(self.meeting, "set_word"):
            self.meeting.set_word(problem)
        else:
            self.meeting.reset_meeting()
            
            # Try different method names that might exist
            if hasattr(self.meeting, "pose_problem"):
                self.meeting.pose_problem(problem)
            elif hasattr(self.meeting, "set_word"):
                self.meeting.set_word(problem)
        
        # Start the meeting
        if hasattr(self.meeting, "start_meeting"):
            self.meeting.start_meeting()
        elif hasattr(self.meeting, "start_game"):
            self.meeting.start_game()
        
        self.switch_to_state(Application.MEETING)
    
    def join_meeting(self):
        """Join an existing meeting"""
        self.clear_screen()
        
        # Draw header
        self.draw_header("Join a Meeting")
        
        # Get server IP
        print()
        server_ip = self.get_input("Enter server IP address: ")
        
        if not server_ip:
            server_ip = "localhost"  # Default to localhost
        
        self.is_host = False
        
        # Try to connect to server
        try:
            self.network = PoE_Client(self.meeting, self, host=server_ip)
            if not self.network.connect():
                self.show_error("Failed to connect to server")
                return
            
            print("\nConnected to server successfully!")
            print("Press any key to continue...")
            msvcrt.getch()
            
        except Exception as e:
            self.show_error(f"Connection error: {e}")
            return
        
        self.switch_to_state(Application.MEETING)
    
    def propose_solution(self, letter):
        """Propose a solution (letter)"""
        if len(letter) != 1 or not letter.isalpha():
            self.show_error("Invalid input! Please enter a single letter.")
            return
        
        if self.network:
            if hasattr(self.network, "propose_solution"):
                self.network.propose_solution(letter.lower())
            elif hasattr(self.network, "send_guess"):
                self.network.send_guess(letter.lower())
        else:
            if hasattr(self.meeting, "guess_letter"):
                self.meeting.guess_letter(letter.lower())
            elif hasattr(self.meeting, "propose_solution"):
                self.meeting.propose_solution(letter.lower())
            self.draw_meeting_view()
    
    def start_new_meeting(self):
        """Start a new meeting (from host)"""
        if self.network:
            if hasattr(self.network, "start_new_meeting"):
                self.network.start_new_meeting()
            elif hasattr(self.network, "start_new_game"):
                self.network.start_new_game()  # Fallback for legacy code
        else:
            if hasattr(self.meeting, "reset_meeting"):
                self.meeting.reset_meeting()
            elif hasattr(self.meeting, "reset_game"):
                self.meeting.reset_game()
                
            if hasattr(self.meeting, "start_meeting"):
                self.meeting.start_meeting()
            elif hasattr(self.meeting, "start_game"):
                self.meeting.start_game()
                
            self.switch_to_state(Application.MEETING)
    
    def show_error(self, message):
        """Show an error message to the user"""
        print(f"\n{Colors.RED}ERROR: {message}{Colors.RESET}")
        print("Press any key to continue...")
        msvcrt.getch()
        
        # Redraw the current screen
        if self.current_state == Application.MEETING:
            self.draw_meeting_view()
        elif self.current_state == Application.MAIN_MENU:
            self.draw_main_menu()
        elif self.current_state == Application.RESULTS:
            meeting_state = self.meeting.get_meeting_state()
            success = meeting_state["state"] == Transcript.WON
            self.draw_results_view(success, meeting_state["word"])
    
    def show_results(self, success, problem):
        """Show the results screen"""
        self.draw_results_view(success, problem)
        self.switch_to_state(Application.RESULTS)
    
    def update(self):
        """Update the UI based on current meeting state"""
        if self.current_state == Application.MEETING:
            meeting_state = self.meeting.get_meeting_state()
            
            if meeting_state["state"] == Transcript.WON:
                self.show_results(True, meeting_state["word"])
            elif meeting_state["state"] == Transcript.LOST:
                self.show_results(False, meeting_state["word"])
            else:
                # Check if we need to redraw
                self.draw_meeting_view()
    
    def run(self):
        """Main application loop"""
        running = True
        
        while running:
            try:
                # Non-blocking input check
                if msvcrt.kbhit():
                    key = msvcrt.getch().decode('utf-8', errors='ignore')
                    
                    # Main menu options
                    if self.current_state == Application.MAIN_MENU:
                        if key == '1':
                            self.host_meeting()
                        elif key == '2':
                            self.join_meeting()
                        elif key == '3' or key.lower() == 'q':
                            running = False
                    
                    # Meeting options
                    elif self.current_state == Application.MEETING:
                        meeting_state = self.meeting.get_meeting_state()
                        
                        if key.lower() == 'm':
                            self.switch_to_state(Application.MAIN_MENU)
                        elif self.is_host and key.lower() == 's':
                            self.start_new_meeting()
                        elif not self.is_host and meeting_state["state"] == Transcript.PLAYING:
                            if key == '\r':  # Enter key
                                if self.input_buffer:
                                    self.propose_solution(self.input_buffer)
                                    self.input_buffer = ""
                                    self.draw_meeting_view()
                            elif key == '\x1b':  # Escape key
                                self.input_buffer = ""
                                self.draw_meeting_view()
                            elif key == '\b':  # Backspace
                                self.input_buffer = self.input_buffer[:-1]
                                print('\b \b', end='', flush=True)
                            elif len(self.input_buffer) < 1 and key.isalpha():
                                self.input_buffer += key.lower()
                                print(key.lower(), end='', flush=True)
                    
                    # Results options
                    elif self.current_state == Application.RESULTS:
                        if key.lower() == 'm':
                            self.switch_to_state(Application.MAIN_MENU)
                
                # Update the UI periodically
                self.update()
                
                # Small delay to prevent CPU hogging
                time.sleep(0.1)
                
            except Exception as e:
                # Display error message
                self.show_error(f"Error: {str(e)}")
        
        # Clean up
        if self.network:
            self.network.stop()
        print("Thanks for using Process of Elimination!")


def main():
    """Main entry point for the application"""
    # Handle command line arguments
    is_host = False
    if len(sys.argv) > 1 and sys.argv[1] == "--host":
        is_host = True
    
    # Create an instance of the meeting logic
    meeting = PoE_Meeting()
    
    try:
        # Create the TUI
        app = ProcessOfEliminationTUI(meeting, is_host)
        
        # Run the application
        app.run()
    except KeyboardInterrupt:
        print("\nApplication terminated by user.")
    except Exception as e:
        print(f"\nAn error occurred: {e}")
        input("Press Enter to exit...")


if __name__ == "__main__":
    main()