import tkinter as tk
from tkinter import ttk, messagebox
from peer import PeerIdentity

class IdentityDialog(tk.Toplevel):
    def __init__(self, parent, identity: PeerIdentity = None, peer_manager=None):
        super().__init__(parent)
        self.title("Peer Identity Management")
        self.geometry("600x500")
        
        self.parent = parent
        self.identity = identity or PeerIdentity()
        self.peer_manager = peer_manager
        self.result = None
        
        self._create_widgets()
        
    def _create_widgets(self):
        # Identity details frame
        details_frame = ttk.LabelFrame(self, text="Your Identity", padding=10)
        details_frame.pack(fill=tk.X, padx=10, pady=5)
        
        # User ID
        ttk.Label(details_frame, text="User ID:").grid(row=0, column=0, sticky=tk.W)
        self.user_id_var = tk.StringVar(value=self.identity.user_id)
        ttk.Entry(details_frame, textvariable=self.user_id_var).grid(row=0, column=1, sticky=tk.EW)
        
        # Display Name
        ttk.Label(details_frame, text="Display Name:").grid(row=1, column=0, sticky=tk.W)
        self.display_name_var = tk.StringVar(value=self.identity.display_name)
        ttk.Entry(details_frame, textvariable=self.display_name_var).grid(row=1, column=1, sticky=tk.EW)
        
        # Read-only info
        ttk.Label(details_frame, text="Peer ID:").grid(row=2, column=0, sticky=tk.W)
        ttk.Entry(details_frame, state='readonly', textvariable=tk.StringVar(value=self.identity.peer_id)).grid(row=2, column=1, sticky=tk.EW)
        
        ttk.Label(details_frame, text="Hostname:").grid(row=3, column=0, sticky=tk.W)
        ttk.Entry(details_frame, state='readonly', textvariable=tk.StringVar(value=self.identity.hostname)).grid(row=3, column=1, sticky=tk.EW)
        
        ttk.Label(details_frame, text="Domain:").grid(row=4, column=0, sticky=tk.W)
        ttk.Entry(details_frame, state='readonly', textvariable=tk.StringVar(value=self.identity.domain)).grid(row=4, column=1, sticky=tk.EW)
        
        ttk.Label(details_frame, text="Public Key:").grid(row=5, column=0, sticky=tk.W)
        ttk.Entry(details_frame, state='readonly', textvariable=tk.StringVar(value=self.identity.public_key)).grid(row=5, column=1, sticky=tk.EW)
        
        # Sharing frame
        sharing_frame = ttk.LabelFrame(self, text="Identity Sharing", padding=10)
        sharing_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # Share string
        ttk.Label(sharing_frame, text="Your sharing string:").pack(anchor=tk.W)
        self.share_text = tk.Text(sharing_frame, height=4, wrap=tk.WORD)
        self.share_text.pack(fill=tk.X)
        self.share_text.insert('1.0', self.identity.to_sharing_string())
        self.share_text.config(state='disabled')  # Use disabled instead of readonly
        
        # Copy button
        copy_btn = ttk.Button(sharing_frame, text="Copy to Clipboard", 
                             command=self._copy_share_string)
        copy_btn.pack(pady=5)
        
        # Import peer identity
        ttk.Label(sharing_frame, text="Import peer identity:").pack(anchor=tk.W, pady=(10,0))
        self.import_text = tk.Text(sharing_frame, height=4, wrap=tk.WORD)
        self.import_text.pack(fill=tk.X)
        
        import_btn = ttk.Button(sharing_frame, text="Import Peer", 
                               command=self._import_peer)
        import_btn.pack(pady=5)
        
        # Buttons
        btn_frame = ttk.Frame(self)
        btn_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Button(btn_frame, text="Save", command=self._save).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=self._cancel).pack(side=tk.RIGHT)

    def _copy_share_string(self):
        """Copy sharing string to clipboard"""
        self.clipboard_clear()
        self.clipboard_append(self.identity.to_sharing_string())
        messagebox.showinfo("Success", "Sharing string copied to clipboard!")

    def _import_peer(self):
        """Import a peer from sharing string"""
        share_string = self.import_text.get('1.0', tk.END).strip()
        imported_identity = PeerIdentity.from_sharing_string(share_string)
        
        if imported_identity:
            if self.peer_manager:
                self.peer_manager.add_or_update_peer(
                    imported_identity.peer_id, 
                    "manual",
                    user_id=imported_identity.user_id,
                    display_name=imported_identity.display_name,
                    hostname=imported_identity.hostname,
                    domain=imported_identity.domain,
                    public_key=imported_identity.public_key
                )
                messagebox.showinfo("Success", 
                    f"Imported peer: {imported_identity.display_name}\n" +
                    f"ID: {imported_identity.peer_id}")
                self.import_text.delete('1.0', tk.END)
            else:
                messagebox.showerror("Error", "No peer manager available")
        else:
            messagebox.showerror("Error", "Invalid sharing string format")

    def _save(self):
        """Save changes to identity"""
        self.identity.user_id = self.user_id_var.get()
        self.identity.display_name = self.display_name_var.get()
        self.result = self.identity
        self.destroy()
        
    def _cancel(self):
        self.destroy()