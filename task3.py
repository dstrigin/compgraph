import tkinter as tk
from tkinter import ttk, messagebox

class ImageHSVConverter:
    def __init__(self, root):
        self.root = root
        self.setup_ui()
    
    def setup_ui(self):
        label = ttk.Label(self.root, text="Задание 3: Преобразование HSV\n\nЭта функция будет реализована позже", 
            font=('Arial', 14), justify=tk.CENTER)
        label.pack(expand=True)