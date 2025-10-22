import tkinter as tk
from tkinter import ttk
import random
import math

class MidpointDisplacement1D:
    def __init__(self, width=1000, height=500):
        self.width = width
        self.height = height
        self.points = []
        self.random = random.Random()
        
    def generate_next_step(self, roughness):
        if len(self.points) == 0:
            left_height = self.height // 2
            right_height = self.height // 2
            self.points.append((0, left_height))
            self.points.append((self.width - 1, right_height))
            return self.points.copy()
        
        if len(self.points) >= self.width:
            return self.points.copy()
            
        new_points = self.points.copy()
        
        for i in range(len(self.points) - 1):
            pl = self.points[i]
            pr = self.points[i + 1]
            
            R = roughness
            l = math.sqrt((pl[0] - pr[0]) ** 2 + (pl[1] - pr[1]) ** 2)
            h = (pl[1] + pr[1]) / 2 + (self.random.random() * 2 - 1) * R * l
            
            h = max(10, min(self.height - 10, h))
            
            midpoint = (int((pl[0] + pr[0]) / 2), int(h))
            
            new_points.insert(i * 2 + 1, midpoint)
        
        self.points = new_points
        return self.points.copy()
    
    def clear(self, left_height=None, right_height=None):
        self.points = []
        if left_height is not None and right_height is not None:
            self.points.append((0, left_height))
            self.points.append((self.width - 1, right_height))
        return self.points.copy()

class MainApplication:
    def __init__(self, root):
        self.root = root
        self.root.title("Генератор рельефа - 1D Midpoint Displacement")
        self.root.configure(bg='#f0f0f0')
        
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        window_width = min(1200, screen_width - 100)
        window_height = min(800, screen_height - 100)
        self.root.geometry(f"{window_width}x{window_height}")
        
        self.canvas_width = window_width - 40
        self.canvas_height = 500
        self.generator = MidpointDisplacement1D(width=self.canvas_width, height=self.canvas_height)
        
        self.create_widgets()
        self.draw_initial_line()
        
    def create_widgets(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        title_label = ttk.Label(main_frame, text="Генератор рельефа методом Midpoint Displacement", 
                               font=('Arial', 14, 'bold'))
        title_label.pack(pady=10)
        
        canvas_container = ttk.Frame(main_frame)
        canvas_container.pack(fill=tk.BOTH, expand=True, pady=10)
        
        self.canvas = tk.Canvas(canvas_container, width=self.canvas_width, height=self.canvas_height, 
                               bg="white", relief=tk.SUNKEN, bd=2)
        self.canvas.pack(pady=10)
        
        control_frame = ttk.Frame(main_frame)
        control_frame.pack(fill=tk.X, pady=10)
        
        left_control_frame = ttk.Frame(control_frame)
        left_control_frame.pack(side=tk.LEFT, padx=20)
        
        right_control_frame = ttk.Frame(control_frame)
        right_control_frame.pack(side=tk.RIGHT, padx=20)
        
        center_control_frame = ttk.Frame(control_frame)
        center_control_frame.pack(side=tk.LEFT, padx=50)
        
        ttk.Label(left_control_frame, text="Начальные параметры:", font=('Arial', 10, 'bold')).grid(row=0, column=0, columnspan=2, pady=5, sticky=tk.W)
        
        ttk.Label(left_control_frame, text="Левая высота:").grid(row=1, column=0, padx=5, pady=2, sticky=tk.W)
        self.left_height_var = tk.IntVar(value=250)
        self.left_height_spin = ttk.Spinbox(
            left_control_frame, from_=50, to=450, width=8, 
            textvariable=self.left_height_var, command=self.height_changed
        )
        self.left_height_spin.grid(row=1, column=1, padx=5, pady=2)
        
        ttk.Label(left_control_frame, text="Правая высота:").grid(row=2, column=0, padx=5, pady=2, sticky=tk.W)
        self.right_height_var = tk.IntVar(value=250)
        self.right_height_spin = ttk.Spinbox(
            left_control_frame, from_=50, to=450, width=8,
            textvariable=self.right_height_var, command=self.height_changed
        )
        self.right_height_spin.grid(row=2, column=1, padx=5, pady=2)
        
        ttk.Label(center_control_frame, text="Параметры генерации:", font=('Arial', 10, 'bold')).grid(row=0, column=0, columnspan=2, pady=5, sticky=tk.W)
        
        ttk.Label(center_control_frame, text="Шероховатость:").grid(row=1, column=0, padx=5, pady=2, sticky=tk.W)
        self.roughness_var = tk.DoubleVar(value=0.5)
        self.roughness_scale = ttk.Scale(
            center_control_frame, from_=0.1, to=1.5, orient=tk.HORIZONTAL,
            variable=self.roughness_var, length=150
        )
        self.roughness_scale.grid(row=1, column=1, padx=5, pady=2)
        
        self.roughness_label = ttk.Label(center_control_frame, text="0.5")
        self.roughness_label.grid(row=1, column=2, padx=5)
        
        self.show_points_var = tk.BooleanVar(value=True)
        self.show_points_cb = ttk.Checkbutton(
            center_control_frame, text="Показывать точки", 
            variable=self.show_points_var, command=self.redraw
        )
        self.show_points_cb.grid(row=2, column=0, columnspan=2, pady=5, sticky=tk.W)
        
        ttk.Label(right_control_frame, text="Управление:", font=('Arial', 10, 'bold')).grid(row=0, column=0, pady=5, sticky=tk.W)
        
        self.next_step_btn = ttk.Button(
            right_control_frame, text="Следующий шаг", command=self.next_step, width=15
        )
        self.next_step_btn.grid(row=1, column=0, pady=5, sticky=tk.EW)
        
        self.clear_btn = ttk.Button(
            right_control_frame, text="Очистить", command=self.clear, width=15
        )
        self.clear_btn.grid(row=2, column=0, pady=5, sticky=tk.EW)
        
        self.roughness_scale.configure(command=self.on_roughness_changed)
        self.left_height_spin.bind('<Return>', lambda e: self.height_changed())
        self.right_height_spin.bind('<Return>', lambda e: self.height_changed())
        
        self.height_controls_enabled = True
        
    def on_roughness_changed(self, event=None):
        self.roughness_label.config(text=f"{self.roughness_var.get():.2f}")
        
    def height_changed(self):
        if self.height_controls_enabled:
            self.clear()
        
    def draw_initial_line(self):
        self.canvas.delete("all")
        left_height = self.left_height_var.get()
        right_height = self.right_height_var.get()
        
        self.canvas.create_line(
            0, left_height, self.canvas_width-1, right_height,
            width=2, fill="blue"
        )
        
    def draw_lines(self, points):
        self.canvas.delete("all")
        
        for i in range(len(points) - 1):
            x1, y1 = points[i]
            x2, y2 = points[i + 1]
            self.canvas.create_line(x1, y1, x2, y2, width=2, fill="blue")
        
        if self.show_points_var.get():
            for x, y in points:
                self.canvas.create_oval(x-2, y-2, x+2, y+2, fill="red", outline="red")
    
    def next_step(self):
        roughness = self.roughness_var.get()
        points = self.generator.generate_next_step(roughness)
        self.draw_lines(points)
        
        if len(points) > 2:
            self.height_controls_enabled = False
            self.left_height_spin.config(state="disabled")
            self.right_height_spin.config(state="disabled")
    
    def clear(self):
        left_height = self.left_height_var.get()
        right_height = self.right_height_var.get()
        points = self.generator.clear(left_height, right_height)
        self.draw_lines(points)
        
        self.height_controls_enabled = True
        self.left_height_spin.config(state="normal")
        self.right_height_spin.config(state="normal")
    
    def redraw(self):
        if len(self.generator.points) > 0:
            self.draw_lines(self.generator.points)
        else:
            self.draw_initial_line()

def main():
    root = tk.Tk()
    app = MainApplication(root)
    root.mainloop()

if __name__ == "__main__":
    main()