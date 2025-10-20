import tkinter as tk

def main():
    root = tk.Toplevel()
    root.title("Задание 3")
    root.geometry("300x300")
    tk.Label(root, text="Кубические сплайны Безье", font=("Arial", 14)).pack(pady=50)
    root.mainloop()

if __name__ == "__main__":
    main()