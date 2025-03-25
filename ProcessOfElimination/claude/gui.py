import tkinter as tk
from tkinter import messagebox, simpledialog
import math
from logic import Transcript

class Application:
    MAIN_MENU = "main_menu"
    MEETING = "meeting"
    RESULTS = "results"

class PoE_UI:
    def __init__(self, root, meeting, is_host=False, network=None):
        self.root = root
        self.meeting = meeting # TODO: rename to 'meeting'
        self.network = network
        self.is_host = is_host
        
        self.root.title("Process of Elimination")
        self.root.geometry("800x600")
        self.root.resizable(True, True)

        # container for all UI states
        self.container = tk.Frame(self.root)
        self.container.pack(fill=tk.BOTH, expand=True)

        self.frames = {}

        self.current_state = None
        
        if self.network and self.network.running:
            self.switch_to_state(Application.MEETING)
        else:
            self.switch_to_state(Application.MAIN_MENU)
    
    def setup_main_menu(self):
        """Create the main menu UI"""
        frame = tk.Frame(self.container, padx=20, pady=20)
        
        # Title
        title_label = tk.Label(
            frame, 
            text="Process of Elimination", 
            font=("Arial", 24, "bold")
        )
        title_label.pack(pady=20)
        
        subtitle_label = tk.Label(
            frame,
            text="A collaborative problem-solving experience",
            font=("Arial", 14)
        )
        subtitle_label.pack(pady=10)
        
        # Buttons frame
        buttons_frame = tk.Frame(frame, pady=30)
        buttons_frame.pack()
        
        host_button = tk.Button(
            buttons_frame, 
            text="Host a Meeting", 
            font=("Arial", 12),
            width=20,
            height=2,
            command=self.host_meeting
        )
        host_button.pack(pady=10)
        
        join_button = tk.Button(
            buttons_frame, 
            text="Join a Meeting", 
            font=("Arial", 12),
            width=20,
            height=2,
            command=self.join_meeting
        )
        join_button.pack(pady=10)
        
        exit_button = tk.Button(
            buttons_frame, 
            text="Exit", 
            font=("Arial", 12),
            width=20,
            height=2,
            command=self.exit_application
        )
        exit_button.pack(pady=10)
        
        # Copyright info or extra details
        footer_label = tk.Label(
            frame,
            text="© 2025 The Watercooler",
            font=("Arial", 8)
        )
        footer_label.pack(side=tk.BOTTOM, pady=10)
        
        return frame
    
    def setup_meeting_ui(self):
        """Create the meeting UI"""
        frame = tk.Frame(self.container, padx=20, pady=20)
        
        # Meeting header
        header_frame = tk.Frame(frame)
        header_frame.pack(fill=tk.X, pady=10)
        
        meeting_label = tk.Label(
            header_frame, 
            text="Ongoing Meeting", 
            font=("Arial", 16, "bold")
        )
        meeting_label.pack(side=tk.LEFT)
        
        # Add meeting status indicator
        self.meeting_status = tk.Label(
            header_frame,
            text="In Progress",
            fg="green",
            font=("Arial", 12)
        )
        self.meeting_status.pack(side=tk.RIGHT)
        
        # Two-column layout
        content_frame = tk.Frame(frame)
        content_frame.pack(fill=tk.BOTH, expand=True, pady=10)
        
        # Left column - Clock and controls
        left_col = tk.Frame(content_frame, padx=10)
        left_col.pack(side=tk.LEFT, fill=tk.Y)
        
        # Clock display
        clock_frame = tk.LabelFrame(left_col, text="Meeting Timer", padx=10, pady=10)
        clock_frame.pack(pady=10)
        
        self.clock_canvas = tk.Canvas(clock_frame, width=200, height=200, bg="white")
        self.clock_canvas.pack()
        
        # Right column - Whiteboard
        right_col = tk.Frame(content_frame, padx=10)
        right_col.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Whiteboard for the problem
        whiteboard_frame = tk.LabelFrame(right_col, text="Problem Analysis", padx=10, pady=10)
        whiteboard_frame.pack(fill=tk.BOTH, expand=True)
        
        # Problem display
        problem_frame = tk.Frame(whiteboard_frame)
        problem_frame.pack(pady=10)
        
        problem_label = tk.Label(
            problem_frame, 
            text="Current Problem:", 
            font=("Arial", 12)
        )
        problem_label.pack()
        
        self.problem_display = tk.Label(
            problem_frame, 
            text="", 
            font=("Arial", 24, "bold")
        )
        self.problem_display.pack(pady=5)
        
        # Create a whiteboard canvas for visualizing solutions
        self.whiteboard_canvas = tk.Canvas(
            whiteboard_frame, 
            width=400, 
            height=200, 
            bg="white",
            highlightbackground="gray", 
            highlightthickness=1
        )
        self.whiteboard_canvas.pack(fill=tk.BOTH, expand=True, pady=10)
        
        # Proposed solutions section
        solutions_frame = tk.LabelFrame(right_col, text="Proposed Solutions", padx=10, pady=10)
        solutions_frame.pack(fill=tk.X, pady=10)
        
        # Two columns for solutions: correct and incorrect
        solutions_columns = tk.Frame(solutions_frame)
        solutions_columns.pack(fill=tk.X, expand=True)
        
        # Correct solutions
        correct_frame = tk.Frame(solutions_columns)
        correct_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        correct_label = tk.Label(
            correct_frame,
            text="Correct Approaches:",
            font=("Arial", 10, "bold"),
            fg="green"
        )
        correct_label.pack(pady=5)
        
        self.correct_solutions = tk.Text(
            correct_frame,
            height=4,
            width=20,
            bg="#f0f9f0",
            state=tk.DISABLED
        )
        self.correct_solutions.pack(fill=tk.BOTH, expand=True)
        
        # Incorrect solutions
        incorrect_frame = tk.Frame(solutions_columns)
        incorrect_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        incorrect_label = tk.Label(
            incorrect_frame,
            text="Incorrect Approaches:",
            font=("Arial", 10, "bold"),
            fg="red"
        )
        incorrect_label.pack(pady=5)
        
        self.incorrect_solutions = tk.Text(
            incorrect_frame,
            height=4,
            width=20,
            bg="#fff0f0",
            state=tk.DISABLED
        )
        self.incorrect_solutions.pack(fill=tk.BOTH, expand=True)
        
        # For employees (clients), create controls for proposing solutions
        if not self.is_host:
            input_frame = tk.Frame(frame)
            input_frame.pack(pady=15)
            
            solution_label = tk.Label(
                input_frame, 
                text="Propose a solution:", 
                font=("Arial", 12)
            )
            solution_label.grid(row=0, column=0, padx=5)
            
            self.solution_entry = tk.Entry(
                input_frame, 
                width=5, 
                font=("Arial", 12)
            )
            self.solution_entry.grid(row=0, column=1, padx=5)
            
            self.propose_button = tk.Button(
                input_frame, 
                text="Propose", 
                command=self.propose_solution
            )
            self.propose_button.grid(row=0, column=2, padx=5)
            
            # Bind Enter key to propose_solution
            self.solution_entry.bind("<Return>", lambda event: self.propose_solution())
        
        # Status message
        self.status_message = tk.Label(
            frame, 
            text="", 
            font=("Arial", 12)
        )
        self.status_message.pack(pady=10)
        
        # Create buttons for host actions (only for host)
        if self.is_host:
            control_frame = tk.Frame(frame)
            control_frame.pack(pady=10)
            
            self.start_button = tk.Button(
                control_frame, 
                text="Start New Meeting", 
                command=self.start_new_meeting
            )
            self.start_button.pack(side=tk.LEFT, padx=5)
        
        return frame
    
    def setup_results_ui(self):
        """Create the results screen UI"""
        frame = tk.Frame(self.container, padx=20, pady=20)
        
        # Results header
        results_label = tk.Label(
            frame, 
            text="Meeting Results", 
            font=("Arial", 20, "bold")
        )
        results_label.pack(pady=20)
        
        # Result message
        self.result_message = tk.Label(
            frame,
            text="",
            font=("Arial", 16)
        )
        self.result_message.pack(pady=10)
        
        # Problem reveal
        problem_frame = tk.Frame(frame)
        problem_frame.pack(pady=20)
        
        problem_label = tk.Label(
            problem_frame,
            text="The problem was:",
            font=("Arial", 12)
        )
        problem_label.pack()
        
        self.final_problem = tk.Label(
            problem_frame,
            text="",
            font=("Arial", 18, "bold")
        )
        self.final_problem.pack(pady=5)
        
        # Navigation buttons
        button_frame = tk.Frame(frame)
        button_frame.pack(pady=30)
        
        main_menu_button = tk.Button(
            button_frame,
            text="Return to Main Menu",
            font=("Arial", 12),
            command=self.return_to_main_menu
        )
        main_menu_button.pack(pady=10)
        
        return frame
    
    def switch_to_state(self, state):
        """Switch the UI to the specified state"""
        if self.current_state == state:
            return
            
        # Hide current frame if it exists
        if self.current_state and self.current_state in self.frames:
            self.frames[self.current_state].pack_forget()
        
        # Create frame if it doesn't exist
        if state not in self.frames:
            if state == Application.MAIN_MENU:
                self.frames[state] = self.setup_main_menu()
            elif state == Application.MEETING:
                self.frames[state] = self.setup_meeting_ui()
            elif state == Application.RESULTS:
                self.frames[state] = self.setup_results_ui()
        
        # Show the new frame
        self.frames[state].pack(fill=tk.BOTH, expand=True)
        self.current_state = state
        
        # Update display if needed
        if state == Application.MEETING:
            self.update_display()
    
    def draw_clock(self, incorrect_guesses):
        """Draw a clock showing time running out instead of process_of_elimination"""
        self.clock_canvas.delete("all")
        
        # Draw clock face
        clock_radius = 80
        center_x, center_y = 100, 100
        
        # Draw outer circle
        self.clock_canvas.create_oval(
            center_x - clock_radius, 
            center_y - clock_radius,
            center_x + clock_radius, 
            center_y + clock_radius, 
            width=3
        )
        
        # Draw clock center
        self.clock_canvas.create_oval(
            center_x - 5, 
            center_y - 5,
            center_x + 5, 
            center_y + 5, 
            fill="black"
        )
        
        # Draw hour marks
        for i in range(12):
            angle = i * 30 * (math.pi/180)  # 30 degrees per hour, convert to radians
            x1 = center_x + (clock_radius - 10) * math.sin(angle)
            y1 = center_y - (clock_radius - 10) * math.cos(angle)
            x2 = center_x + clock_radius * math.sin(angle)
            y2 = center_y - clock_radius * math.cos(angle)
        
        self.clock_canvas.create_line(x1, y1, x2, y2, width=2)
    
        # Calculate the clock hand position based on incorrect guesses
        max_incorrect = self.meeting.get_meeting_state()["max_incorrect"]
        progress = incorrect_guesses / max_incorrect if max_incorrect > 0 else 0
        
        # Calculate angle (0 = 12 o'clock, 0.5 = 6 o'clock, 1.0 = 12 o'clock again)
        angle = progress * 2 * math.pi
        
        # Draw minute hand (always points to current progress)
        minute_length = clock_radius * 0.7
        minute_x = center_x + minute_length * math.sin(angle)
        minute_y = center_y - minute_length * math.cos(angle)
        self.clock_canvas.create_line(center_x, center_y, minute_x, minute_y, width=3, fill="blue")
        
        # Draw hour hand (moves at 1/12 the speed of minute hand)
        hour_length = clock_radius * 0.5
        hour_angle = (progress * 2 * math.pi) / 12
        hour_x = center_x + hour_length * math.sin(hour_angle)
        hour_y = center_y - hour_length * math.cos(hour_angle)
        self.clock_canvas.create_line(center_x, center_y, hour_x, hour_y, width=4, fill="black")
        
        # Add indicators of time running out
        if incorrect_guesses >= max_incorrect * 0.75:
            # Red background for urgent time
            self.clock_canvas.create_oval(
                center_x - clock_radius, 
                center_y - clock_radius,
                center_x + clock_radius, 
                center_y + clock_radius, 
                fill="pink", outline="", tags="bg"
            )
            self.clock_canvas.tag_lower("bg")  # Put background behind everything else
            
            # Exclamation mark for urgency
            self.clock_canvas.create_text(
                center_x, center_y + 30,
                text="Time running out!",
                fill="red",
                font=("Arial", 10, "bold")
            )
    
    def update_display(self):
        """Update the UI based on current meeting state"""
        if self.current_state != Application.MEETING:
            return
            
        meeting_state = self.meeting.get_meeting_state()
        
        # Update clock drawing
        self.draw_clock(meeting_state["incorrect_guesses"])
        
        # Update problem display
        self.problem_display.config(text=meeting_state["display_word"])
        
        # Update solution lists on whiteboard
        correct = []
        incorrect = []
        for letter in meeting_state["guessed_letters"]:
            if letter in meeting_state["word"]:
                correct.append(letter)
            else:
                incorrect.append(letter)
        
        # Update correct solutions text
        self.correct_solutions.config(state=tk.NORMAL)
        self.correct_solutions.delete(1.0, tk.END)
        self.correct_solutions.insert(tk.END, ", ".join(correct) if correct else "None yet")
        self.correct_solutions.config(state=tk.DISABLED)
        
        # Update incorrect solutions text
        self.incorrect_solutions.config(state=tk.NORMAL)
        self.incorrect_solutions.delete(1.0, tk.END)
        self.incorrect_solutions.insert(tk.END, ", ".join(incorrect) if incorrect else "None yet")
        self.incorrect_solutions.config(state=tk.DISABLED)
        
        # Draw solution visualization on whiteboard
        self.update_whiteboard(meeting_state)
        
        # Update status message and UI state
        if meeting_state["state"] == Transcript.WAITING:
            self.status_message.config(text="Waiting for meeting to start...")
            self.meeting_status.config(text="Not Started", fg="orange")
            if not self.is_host and hasattr(self, "solution_entry"):
                self.solution_entry.config(state=tk.DISABLED)
                self.propose_button.config(state=tk.DISABLED)
                
        elif meeting_state["state"] == Transcript.PLAYING:
            time_remaining = meeting_state["max_incorrect"] - meeting_state["incorrect_guesses"]
            self.status_message.config(
                text=f"Time remaining: {time_remaining}/{meeting_state['max_incorrect']} units"
            )
            self.meeting_status.config(text="In Progress", fg="green")
            if not self.is_host and hasattr(self, "solution_entry"):
                self.solution_entry.config(state=tk.NORMAL)
                self.propose_button.config(state=tk.NORMAL)
                
        elif meeting_state["state"] == Transcript.WON:
            self.meeting_status.config(text="Completed", fg="blue")
            self.show_results(True, meeting_state["word"])
            
        elif meeting_state["state"] == Transcript.LOST:
            self.meeting_status.config(text="Failed", fg="red")
            self.show_results(False, meeting_state["word"])

    def update_whiteboard(self, meeting_state):
        """Draw a business diagram on the whiteboard canvas"""
        self.whiteboard_canvas.delete("all")
        
        # Draw a business-like diagram showing the problem solution progress
        width = self.whiteboard_canvas.winfo_width()
        height = self.whiteboard_canvas.winfo_height()
        
        # Draw whiteboard background with faint grid
        for i in range(0, width, 20):
            self.whiteboard_canvas.create_line(
                i, 0, i, height, 
                fill="#e0e0e0"
            )
        
        for i in range(0, height, 20):
            self.whiteboard_canvas.create_line(
                0, i, width, i, 
                fill="#e0e0e0"
            )
        
        # Draw a process flow diagram for the problem
        progress = len([c for c in meeting_state["display_word"] if c.isalpha()]) / len(meeting_state["word"])
        
        # Draw box for problem statement
        self.whiteboard_canvas.create_rectangle(
            50, 40, 350, 80, 
            fill="#d0e0ff", outline="black"
        )
        self.whiteboard_canvas.create_text(
            200, 60,
            text="Problem Analysis Process",
            font=("Arial", 12, "bold")
        )
        
        # Draw arrow down
        self.whiteboard_canvas.create_line(
            200, 80, 200, 110,
            arrow=tk.LAST, width=2
        )
        
        # Draw process box
        progress_fill = f"#{int(255*(1-progress)):02x}{int(255*progress):02x}80"
        self.whiteboard_canvas.create_rectangle(
            50, 110, 350, 150,
            fill=progress_fill, outline="black"
        )
        
        # Progress text
        percent = int(progress * 100)
        self.whiteboard_canvas.create_text(
            200, 130,
            text=f"Solution Progress: {percent}%",
            font=("Arial", 11)
        )
        
        # Draw remaining choices indicator
        remaining = meeting_state["max_incorrect"] - meeting_state["incorrect_guesses"]
        total = meeting_state["max_incorrect"]
        
        self.whiteboard_canvas.create_text(
            200, 170,
            text=f"Remaining time: {remaining}/{total}",
            font=("Arial", 10),
            fill="red" if remaining < 3 else "black"
        )
    
    def show_results(self, success, problem):
        """Show the results screen"""
        self.switch_to_state(Application.RESULTS)
        
        if success:
            self.result_message.config(
                text="Problem solved successfully!",
                fg="green"
            )
        else:
            self.result_message.config(
                text="Time expired before problem was solved.",
                fg="red"
            )
            
        self.final_problem.config(text=problem)
    
    def propose_solution(self):
        """Handle employee solution proposal (formerly make_guess)"""
        letter = self.solution_entry.get().strip()
        if len(letter) != 1 or not letter.isalpha():
            messagebox.showwarning("Invalid Input", "Please enter a single letter solution.")
            self.solution_entry.delete(0, tk.END)
            return
        
        # Clear entry field
        self.solution_entry.delete(0, tk.END)
        
        if self.network:
            # Send solution over network
            # TODO: Rename suggest_solution to send_solution in network.py
            self.network.propose_solution(letter)
        else:
            # Local meetingplay
            self.meeting.guess_letter(letter)
            self.update_display()
    
    def start_new_meeting(self):
        """Start a new meeting (formerly start_new_meeting)"""
        if self.network:
            # TODO: Rename start_new_meeting to start_new_meeting in network.py
            self.network.start_new_meeting()
        else:
            self.meeting.reset_meeting()
            self.meeting.start_meeting()
            self.switch_to_state(Application.MEETING)
    
    def host_meeting(self):
        """Host a new meeting"""
        problem = simpledialog.askstring("Schedule a New Meeting", "Enter a problem for the team to solve:")

        if not problem:
            return
        
        self.is_host = True

        if hasattr(self.meeting, "pose_problem"):
            self.meeting.pose_problem(problem)
        else:
            self.meeting.reset_meeting()
            self.meeting.pose_problem(problem)

        self.meeting.start_meeting()

        self.switch_to_state(Application.MEETING)
    
    def join_meeting(self):
        """Join an existing meeting"""
        # TODO: This should create a process_of_eliminationClient (rename to MeetingClient)
        # For now, we'll just switch to the meeting UI
        self.is_host = False
        self.switch_to_state(Application.MEETING)
    
    def return_to_main_menu(self):
        """Return to the main menu"""
        self.switch_to_state(Application.MAIN_MENU)
    
    def exit_application(self):
        """Exit the application"""
        if self.network:
            self.network.stop()
        self.root.destroy()


# TODO: Rename this class across the codebase
HangmanUI = PoE_UI  # For backward compatibility