import tkinter as tk
from tkinter import ttk, colorchooser, messagebox
from PIL import Image, ImageTk, ImageDraw

class FadeFiller:
    def __init__(self, root):
        self.root = root
        self.root.title("Градиентное окрашивание")
        self.root.geometry("1000x750+200+100")
        self.root.minsize(800, 600)
        
        self.vertex_colors = ["#FF0000", "#00FF00", "#0000FF"]
        self.vertices = []
        self.triangle_drawn = False
        
        self.create_widgets()
        self.init_canvas_image()
        
    def create_widgets(self):
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        left_frame = ttk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        ttk.Label(left_frame, text="Область для рисования треугольника", 
                 font=('Arial', 12)).pack(pady=5)
        
        self.canvas = tk.Canvas(left_frame, bg="white", width=500, height=450)
        self.canvas.pack(pady=10)
        
        self.canvas.bind("<Button-1>", self.add_vertex)
        
        right_frame = ttk.Frame(main_frame, width=300)
        right_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(20, 0))
        right_frame.pack_propagate(False)
        
        ttk.Label(right_frame, text="Управление цветами", 
                 font=('Arial', 14)).pack(pady=10)
        
        colors_frame = ttk.LabelFrame(right_frame, text="Цвета вершин", padding=10)
        colors_frame.pack(fill=tk.X, pady=10)
        
        vertex1_frame = ttk.Frame(colors_frame)
        vertex1_frame.pack(fill=tk.X, pady=5)
        ttk.Button(vertex1_frame, text="Вершина 1", 
                  command=lambda: self.choose_color(0)).pack(side=tk.LEFT)
        self.color_label1 = tk.Label(vertex1_frame, bg=self.vertex_colors[0], 
                                   width=6, height=1, relief='sunken')
        self.color_label1.pack(side=tk.RIGHT, padx=5)
        
        vertex2_frame = ttk.Frame(colors_frame)
        vertex2_frame.pack(fill=tk.X, pady=5)
        ttk.Button(vertex2_frame, text="Вершина 2", 
                  command=lambda: self.choose_color(1)).pack(side=tk.LEFT)
        self.color_label2 = tk.Label(vertex2_frame, bg=self.vertex_colors[1], 
                                   width=6, height=1, relief='sunken')
        self.color_label2.pack(side=tk.RIGHT, padx=5)
        
        vertex3_frame = ttk.Frame(colors_frame)
        vertex3_frame.pack(fill=tk.X, pady=5)
        ttk.Button(vertex3_frame, text="Вершина 3", 
                  command=lambda: self.choose_color(2)).pack(side=tk.LEFT)
        self.color_label3 = tk.Label(vertex3_frame, bg=self.vertex_colors[2], 
                                   width=6, height=1, relief='sunken')
        self.color_label3.pack(side=tk.RIGHT, padx=5)
        
        control_frame = ttk.LabelFrame(right_frame, text="Управление", padding=10)
        control_frame.pack(fill=tk.X, pady=10)
        
        ttk.Button(control_frame, text="Нарисовать треугольник", 
                  command=self.draw_triangle_outline).pack(fill=tk.X, pady=5)
        
        ttk.Button(control_frame, text="Раскрасить градиентом", 
                  command=self.fill_gradient).pack(fill=tk.X, pady=5)
        
        ttk.Button(control_frame, text="Очистить холст", 
                  command=self.clear_canvas).pack(fill=tk.X, pady=5)
        
        info_frame = ttk.LabelFrame(right_frame, text="Инструкция", padding=10)
        info_frame.pack(fill=tk.X, pady=10)
        
        info_text = """1. Кликните 3 раза на холсте чтобы задать вершины треугольника
2. Нажмите 'Нарисовать треугольник'
3. Выберите цвета для вершин
4. Нажмите 'Раскрасить градиентом'"""
        
        self.info_label = ttk.Label(info_frame, text=info_text, 
                                   wraplength=250, justify=tk.LEFT)
        self.info_label.pack()
        
        self.status_label = ttk.Label(right_frame, text="Кликните 3 точки для вершин треугольника", 
                                     relief=tk.SUNKEN)
        self.status_label.pack(side=tk.BOTTOM, fill=tk.X, pady=5)
    
    def init_canvas_image(self):
        self.canvas_width = 500
        self.canvas_height = 450
        self.image = Image.new("RGB", (self.canvas_width, self.canvas_height), "white")
        self.draw = ImageDraw.Draw(self.image)
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas_image = self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)
    
    def add_vertex(self, event):
        if len(self.vertices) < 3:
            x, y = event.x, event.y
            self.vertices.append((x, y))
            
            self.canvas.create_oval(x-3, y-3, x+3, y+3, fill="red", outline="black")
            
            self.status_label.config(text=f"Вершина {len(self.vertices)}: ({x}, {y})")
            
            if len(self.vertices) == 3:
                self.status_label.config(text="Все 3 вершины заданы. Нажмите 'Нарисовать треугольник'")
    
    def choose_color(self, vertex_index):
        color_tuple = colorchooser.askcolor(title=f"Выберите цвет для вершины {vertex_index + 1}")
        if color_tuple and color_tuple[1]:
            self.vertex_colors[vertex_index] = color_tuple[1]
            
            color_labels = [self.color_label1, self.color_label2, self.color_label3]
            color_labels[vertex_index].config(bg=color_tuple[1])
            
            self.status_label.config(text=f"Вершина {vertex_index + 1}: {color_tuple[1]}")
    
    def draw_triangle_outline(self):
        if len(self.vertices) != 3:
            messagebox.showwarning("Внимание", f"Нужно 3 вершины, а задано: {len(self.vertices)}")
            return
        
        try:
            v1, v2, v3 = self.vertices
            
            self.image = Image.new("RGB", (self.canvas_width, self.canvas_height), "white")
            self.draw = ImageDraw.Draw(self.image)
            self.photo = ImageTk.PhotoImage(self.image)
            self.canvas.delete("all")
            self.canvas_image = self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)
            
            for x, y in self.vertices:
                self.canvas.create_oval(x-3, y-3, x+3, y+3, fill="red", outline="black")
            
            self.canvas.create_line(v1[0], v1[1], v2[0], v2[1], width=2, fill="black")
            self.canvas.create_line(v2[0], v2[1], v3[0], v3[1], width=2, fill="black")
            self.canvas.create_line(v3[0], v3[1], v1[0], v1[1], width=2, fill="black")
            
            self.draw.line([v1[0], v1[1], v2[0], v2[1]], fill="black", width=2)
            self.draw.line([v2[0], v2[1], v3[0], v3[1]], fill="black", width=2)
            self.draw.line([v3[0], v3[1], v1[0], v1[1]], fill="black", width=2)
            
            self.photo = ImageTk.PhotoImage(self.image)
            self.canvas.itemconfig(self.canvas_image, image=self.photo)
            
            self.triangle_drawn = True
            self.status_label.config(text="Треугольник нарисован. Выберите цвета и раскрасьте.")
            
        except ValueError as e:
            messagebox.showerror("Ошибка", f"Ошибка при рисовании треугольника: {e}")

    def clear_canvas(self):
        """Очистка холста"""
        self.image = Image.new("RGB", (self.canvas_width, self.canvas_height), "white")
        self.draw = ImageDraw.Draw(self.image)
        self.photo = ImageTk.PhotoImage(self.image)
        self.canvas.delete("all")
        self.canvas_image = self.canvas.create_image(0, 0, anchor=tk.NW, image=self.photo)
        self.vertices = []
        self.triangle_drawn = False
        self.status_label.config(text="Холст очищен. Кликните 3 точки для вершин.")

    def fill_gradient(self):
        if not self.triangle_drawn or len(self.vertices) != 3:
            messagebox.showwarning("Внимание", "Сначала нарисуйте треугольник (нажмите 'Нарисовать треугольник')")
            return
        
        try:
            v1, v2, v3 = self.vertices
            
            color1 = self.hex_to_rgb(self.vertex_colors[0])
            color2 = self.hex_to_rgb(self.vertex_colors[1])
            color3 = self.hex_to_rgb(self.vertex_colors[2])
            
            pixels = self.image.load()
            
            x_min = max(0, min(v1[0], v2[0], v3[0]))
            x_max = min(self.canvas_width - 1, max(v1[0], v2[0], v3[0]))
            y_min = max(0, min(v1[1], v2[1], v3[1]))
            y_max = min(self.canvas_height - 1, max(v1[1], v2[1], v3[1]))
            
            detT = (v2[1] - v3[1]) * (v1[0] - v3[0]) + (v3[0] - v2[0]) * (v1[1] - v3[1])
            
            if detT == 0:
                messagebox.showerror("Ошибка", "Треугольник вырожденный")
                return
            
            for y in range(int(y_min), int(y_max) + 1):
                for x in range(int(x_min), int(x_max) + 1):
                    w1 = ((v2[1] - v3[1]) * (x - v3[0]) + (v3[0] - v2[0]) * (y - v3[1])) / detT
                    w2 = ((v3[1] - v1[1]) * (x - v3[0]) + (v1[0] - v3[0]) * (y - v3[1])) / detT
                    w3 = 1 - w1 - w2
                    
                    if w1 >= 0 and w2 >= 0 and w3 >= 0:
                        r = int(w1 * color1[0] + w2 * color2[0] + w3 * color3[0])
                        g = int(w1 * color1[1] + w2 * color2[1] + w3 * color3[1])
                        b = int(w1 * color1[2] + w2 * color2[2] + w3 * color3[2])
                        
                        r = max(0, min(255, r))
                        g = max(0, min(255, g))
                        b = max(0, min(255, b))
                        
                        pixels[x, y] = (r, g, b)
            
            # Обновляем холст
            self.photo = ImageTk.PhotoImage(self.image)
            self.canvas.itemconfig(self.canvas_image, image=self.photo)
            
            self.status_label.config(text="Треугольник закрашен градиентом")
            
        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка при заливке: {e}")

    def hex_to_rgb(self, hex_color):
        hex_color = hex_color.lstrip('#')
        return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
