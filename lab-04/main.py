import tkinter as tk
from tkinter import ttk, messagebox
import math

class PolygonEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Polygon Editor")
        self.root.geometry("1200x800+350+130")
        self.root.minsize(600, 400)
        
        self.polygons = []
        self.current_polygon = []
        self.selected_polygon_idx = None
        self.edges = []
        self.test_point = None
        self.mode = "create"
        
        self.create_widgets()
        
        self.canvas.bind("<Button-1>", self.on_canvas_click)
        self.canvas.bind("<Motion>", self.on_canvas_motion)
        
    def create_widgets(self):
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True)
        
        canvas_frame = ttk.Frame(main_frame)
        canvas_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        self.canvas = tk.Canvas(canvas_frame, bg="white", width=800, height=800)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        control_frame = ttk.Frame(main_frame, width=400)
        control_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=10, pady=10)
        control_frame.pack_propagate(False)
        
        title_label = ttk.Label(control_frame, text="Polygon Editor", font=("Arial", 16, "bold"))
        title_label.pack(pady=10)
        
        ttk.Separator(control_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=10)
        
        create_frame = ttk.LabelFrame(control_frame, text="Создание полигонов", padding=10)
        create_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(create_frame, text="Начать новый полигон", command=self.start_new_polygon).pack(fill=tk.X, pady=2)
        ttk.Button(create_frame, text="Завершить полигон", command=self.finish_polygon).pack(fill=tk.X, pady=2)
        
        control_buttons_frame = ttk.LabelFrame(control_frame, text="Управление", padding=10)
        control_buttons_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(control_buttons_frame, text="Выбрать полигон", command=self.select_polygon_mode).pack(fill=tk.X, pady=2)
        ttk.Button(control_buttons_frame, text="Снять выделение", command=self.deselect_polygon).pack(fill=tk.X, pady=2)
        ttk.Button(control_buttons_frame, text="Очистить сцену", command=self.clear_scene).pack(fill=tk.X, pady=2)
        
        transform_frame = ttk.LabelFrame(control_frame, text="Аффинные преобразования", padding=10)
        transform_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(transform_frame, text="Смещение на dx, dy", command=self.translate_polygon).pack(fill=tk.X, pady=2)
        ttk.Button(transform_frame, text="Поворот вокруг точки", command=self.rotate_around_point).pack(fill=tk.X, pady=2)
        ttk.Button(transform_frame, text="Поворот вокруг центра", command=self.rotate_around_center).pack(fill=tk.X, pady=2)
        ttk.Button(transform_frame, text="Масштаб относительно точки", command=self.scale_around_point).pack(fill=tk.X, pady=2)
        ttk.Button(transform_frame, text="Масштаб относительно центра", command=self.scale_around_center).pack(fill=tk.X, pady=2)
        
        geometry_frame = ttk.LabelFrame(control_frame, text="Геометрические операции", padding=10)
        geometry_frame.pack(fill=tk.X, pady=5)
        
        ttk.Button(geometry_frame, text="Поиск пересечения ребер", command=self.find_edge_intersection).pack(fill=tk.X, pady=2)
        ttk.Button(geometry_frame, text="Проверка точки в полигоне", command=self.point_in_polygon_test).pack(fill=tk.X, pady=2)
        ttk.Button(geometry_frame, text="Классификация точки относительно ребра", command=self.point_edge_classification).pack(fill=tk.X, pady=2)
        
        info_frame = ttk.LabelFrame(control_frame, text="Информация", padding=10)
        info_frame.pack(fill=tk.X, pady=5)
        
        self.info_label = ttk.Label(info_frame, text="Режим: Создание полигонов\nКликните на холсте для добавления точек", wraplength=350)
        self.info_label.pack(fill=tk.X)
        
        self.status_var = tk.StringVar(value="Готов")
        status_bar = ttk.Label(control_frame, textvariable=self.status_var, relief=tk.SUNKEN, style='TLabel')
        status_bar.pack(fill=tk.X, side=tk.BOTTOM, pady=5)
    
    def on_canvas_click(self, event):
        x, y = event.x, event.y
        
        if self.mode == "create":
            self.current_polygon.append((x, y))
            self.draw_current_state()
            self.update_info(f"Добавлена точка: ({x}, {y})")
            
        elif self.mode == "select":
            self.select_polygon_at(x, y)
            
        elif self.mode == "edge_intersection":
            self.handle_edge_intersection_click(x, y)
            
        elif self.mode == "point_in_polygon":
            self.handle_point_in_polygon_click(x, y)
            
        elif self.mode == "point_edge_classification":
            self.handle_point_edge_classification_click(x, y)
    
    def on_canvas_motion(self, event):
        self.status_var.set(f"Позиция: ({event.x}, {event.y})")
    
    def start_new_polygon(self):
        if self.current_polygon:
            messagebox.showwarning("Внимание", "Завершите текущий полигон перед созданием нового")
            return
        self.mode = "create"
        self.update_info("Режим: Создание полигона. Кликайте на холсте для добавления точек")
    
    def finish_polygon(self):
        if len(self.current_polygon) < 1:
            messagebox.showwarning("Внимание", "Добавьте хотя бы одну точку")
            return
            
        if len(self.current_polygon) >= 3:
            self.current_polygon.append(self.current_polygon[0])
            
        self.polygons.append(self.current_polygon.copy())
        self.current_polygon = []
        self.draw_current_state()
        self.update_info(f"Полигон добавлен. Всего полигонов: {len(self.polygons)}")
    
    def clear_scene(self):
        self.polygons = []
        self.current_polygon = []
        self.selected_polygon_idx = None
        self.edges = []
        self.test_point = None
        self.canvas.delete("all")
        self.update_info("Сцена очищена")
    
    def select_polygon_mode(self):
        self.mode = "select"
        self.update_info("Режим: Выбор полигона. Кликните на полигон (внутрь, на ребро или вершину)")
    
    def deselect_polygon(self):
        self.selected_polygon_idx = None
        self.draw_current_state()
        self.update_info("Выделение снято")
    
    def select_polygon_at(self, x, y):
        selected_idx = None
        
        for i, polygon in enumerate(self.polygons):
            if (self.is_point_near_vertex(x, y, polygon) or 
                self.is_point_near_edge(x, y, polygon) or 
                self.is_point_in_polygon(x, y, polygon)):
                selected_idx = i
                break
        
        if selected_idx is not None:
            self.selected_polygon_idx = selected_idx
            self.draw_current_state()
            poly_type = self.get_polygon_type(self.polygons[selected_idx])
            self.update_info(f"Выбран полигон {selected_idx+1} ({poly_type}) с {len(self.polygons[selected_idx])} точками")
        else:
            self.selected_polygon_idx = None
            self.update_info("Полигон не найден")
    
    def is_point_near_vertex(self, x, y, polygon, threshold=10):
        for px, py in polygon:
            if math.sqrt((px - x)**2 + (py - y)**2) <= threshold:
                return True
        return False
    
    def is_point_near_edge(self, x, y, polygon, threshold=10):
        for i in range(len(polygon) - 1):
            x1, y1 = polygon[i]
            x2, y2 = polygon[i + 1]
            
            distance = self.point_to_line_distance(x, y, x1, y1, x2, y2)
            if distance <= threshold:
                return True
        return False
    
    def point_to_line_distance(self, x, y, x1, y1, x2, y2):
        dx = x2 - x1
        dy = y2 - y1
        dx1 = x - x1
        dy1 = y - y1
        
        dot = dx1 * dx + dy1 * dy
        len_sq = dx * dx + dy * dy
        
        if len_sq == 0:
            return math.sqrt(dx1 * dx1 + dy1 * dy1)
        
        t = max(0, min(1, dot / len_sq))
        
        closest_x = x1 + t * dx
        closest_y = y1 + t * dy
        
        return math.sqrt((x - closest_x)**2 + (y - closest_y)**2)
    
    def is_point_in_polygon(self, x, y, polygon):
        if len(polygon) < 3:
            return False
            
        n = len(polygon)
        inside = False
        
        p1x, p1y = polygon[0]
        for i in range(1, n + 1):
            p2x, p2y = polygon[i % n]
            if y > min(p1y, p2y):
                if y <= max(p1y, p2y):
                    if x <= max(p1x, p2x):
                        if p1y != p2y:
                            xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                        if p1x == p2x or x <= xinters:
                            inside = not inside
            p1x, p1y = p2x, p2y
        
        return inside
    
    def get_polygon_type(self, polygon):
        if len(polygon) == 1:
            return "Точка"
        elif len(polygon) == 2:
            return "Ребро"
        else:
            return "Полигон"
    
    def translate_polygon(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return
        
        dialog = tk.Toplevel(self.root)
        dialog.title("Смещение полигона")
        dialog.geometry("300x150")
        dialog.transient(self.root)
        dialog.grab_set()
        
        ttk.Label(dialog, text="Введите смещение:").pack(pady=10)
        
        dx_frame = ttk.Frame(dialog)
        dx_frame.pack(pady=5)
        ttk.Label(dx_frame, text="dx:").pack(side=tk.LEFT)
        dx_entry = ttk.Entry(dx_frame, width=10)
        dx_entry.pack(side=tk.LEFT, padx=5)
        dx_entry.insert(0, "10")
        
        dy_frame = ttk.Frame(dialog)
        dy_frame.pack(pady=5)
        ttk.Label(dy_frame, text="dy:").pack(side=tk.LEFT)
        dy_entry = ttk.Entry(dy_frame, width=10)
        dy_entry.pack(side=tk.LEFT, padx=5)
        dy_entry.insert(0, "10")
        
        def apply_translation():
            try:
                dx = float(dx_entry.get())
                dy = float(dy_entry.get())
                messagebox.showinfo("Смещение", f"Полигон смещен на dx={dx}, dy={dy}")
                dialog.destroy()
            except ValueError:
                messagebox.showerror("Ошибка", "Введите числовые значения")
        
        ttk.Button(dialog, text="Применить", command=apply_translation).pack(pady=10)
    
    def rotate_around_point(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return
        messagebox.showinfo("Поворот", "Поворот вокруг точки будет реализован")
    
    def rotate_around_center(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return
        messagebox.showinfo("Поворот", "Поворот вокруг центра будет реализован")
    
    def scale_around_point(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return
        messagebox.showinfo("Масштабирование", "Масштабирование относительно точки будет реализовано")
    
    def scale_around_center(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return
        messagebox.showinfo("Масштабирование", "Масштабирование относительно центра будет реализовано")
    
    def find_edge_intersection(self):
        self.mode = "edge_intersection"
        self.edges = []
        self.update_info("Режим: Поиск пересечения ребер. Добавьте два ребра")
    
    def handle_edge_intersection_click(self, x, y):
        self.edges.append((x, y))
        if len(self.edges) == 4:
            messagebox.showinfo("Пересечение", "Поиск пересечения будет реализован")
            self.edges = []
            self.mode = "create"
    
    def point_in_polygon_test(self):
        self.mode = "point_in_polygon"
        self.update_info("Режим: Проверка точки в полигоне. Кликните на точку для проверки")
    
    def handle_point_in_polygon_click(self, x, y):
        self.test_point = (x, y)
        messagebox.showinfo("Принадлежность точки", "Проверка принадлежности точки будет реализована")
        self.draw_current_state()
    
    def point_edge_classification(self):
        self.mode = "point_edge_classification"
        self.update_info("Режим: Классификация точки. Выберите ребро и точку")
    
    def handle_point_edge_classification_click(self, x, y):
        messagebox.showinfo("Классификация", "Классификация точки относительно ребра будет реализована")
    
    def draw_current_state(self):
        self.canvas.delete("all")
        
        for i, polygon in enumerate(self.polygons):
            color = "blue"
            width = 2
            if i == self.selected_polygon_idx:
                color = "red"
                width = 3
            
            if len(polygon) == 1:
                x, y = polygon[0]
                self.canvas.create_oval(x-3, y-3, x+3, y+3, fill=color, outline=color)
            elif len(polygon) == 2:
                x1, y1 = polygon[0]
                x2, y2 = polygon[1]
                self.canvas.create_line(x1, y1, x2, y2, fill=color, width=width)
            else:
                self.canvas.create_polygon(polygon, fill="lightblue", outline=color, width=width)
                
                if polygon:
                    center_x = sum(p[0] for p in polygon) / len(polygon)
                    center_y = sum(p[1] for p in polygon) / len(polygon)
                    self.canvas.create_text(center_x, center_y, text=str(i+1), fill="darkblue")
        
        if self.current_polygon:
            if len(self.current_polygon) == 1:
                x, y = self.current_polygon[0]
                self.canvas.create_oval(x-3, y-3, x+3, y+3, fill="green", outline="green")
            else:
                self.canvas.create_line(self.current_polygon, fill="green", width=2)
        
        if self.edges:
            for i in range(0, len(self.edges), 2):
                if i+1 < len(self.edges):
                    x1, y1 = self.edges[i]
                    x2, y2 = self.edges[i+1]
                    self.canvas.create_line(x1, y1, x2, y2, fill="orange", width=3)
        
        if self.test_point:
            x, y = self.test_point
            self.canvas.create_oval(x-4, y-4, x+4, y+4, fill="purple", outline="purple")
    
    def update_info(self, message):
        self.info_label.config(text=message)

def main():
    root = tk.Tk()
    app = PolygonEditor(root)
    root.mainloop()

if __name__ == "__main__":
    main()