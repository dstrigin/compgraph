import tkinter as tk
from tkinter import ttk, messagebox
import task1
import task2
import task3

class MainMenu:
    def __init__(self, root):
        self.root = root
        self.root.title("Обработка изображений - Главное меню")
        self.root.geometry("800x600+550+250")
        self.root.minsize(400, 300)
        self.root.configure(bg='#f0f0f0')
        
        self.setup_ui()
    
    def setup_ui(self):
        title_label = ttk.Label(self.root, text="Выберите задание", 
            font=('Arial', 16, 'bold'))
        title_label.pack(pady=20)
        
        button_frame = ttk.Frame(self.root)
        button_frame.pack(pady=30)
        
        task1_btn = ttk.Button(button_frame, text="Задание 1: Преобразование в оттенки серого", 
            command=self.open_task1, width=30)
        task1_btn.pack(pady=10)
        
        task2_btn = ttk.Button(button_frame, text="Задание 2: Анализ цветовых каналов", 
            command=self.open_task2, width=30)
        task2_btn.pack(pady=10)
        
        task3_btn = ttk.Button(button_frame, text="Задание 3: Преобразование HSV", 
            command=self.open_task3, width=30)
        task3_btn.pack(pady=10)
        
        info_label = ttk.Label(self.root, text="Для начала работы выберите одно из заданий", 
            font=('Arial', 10))
        info_label.pack(pady=20)
    
    def open_task1(self):
        self.open_task_window("Преобразование в оттенки серого", task1.ImageGrayConverter)
    
    def open_task2(self):
        self.open_task_window("Анализ цветовых каналов", task2.ImageChannelViewer)
    
    def open_task3(self):
        self.open_task_window("Преобразование HSV", task3.ImageHSVConverter)
    
    def open_task_window(self, title, task_class):
        try:
            task_window = tk.Toplevel(self.root)
            task_window.title(title)
            task_window.geometry("1200x800")
            
            task_class(task_window)
            
        except Exception as e:
            messagebox.showerror("Ошибка", f"Не удалось открыть задание: {str(e)}")

def main():
    root = tk.Tk()
    app = MainMenu(root)
    root.mainloop()

if __name__ == "__main__":
    main()