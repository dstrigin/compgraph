import tkinter as tk
import task_2
import task_3

class MainMenu:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Главное меню")
        self.root.geometry("300x300")
        
        tk.Button(self.root, text="Задание 2", command=task_2.main, width=20).pack(pady=10)
        tk.Button(self.root, text="Задание 3", command=task_3.main, width=20).pack(pady=10)
        tk.Button(self.root, text="Выход", command=self.root.quit, width=20).pack(pady=10)
        
    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    MainMenu().run()