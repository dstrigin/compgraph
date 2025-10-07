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

        # Атрибуты для поиска пересечения ребер
        self.edges = []
        self.intersection_point = None
        self.dynamic_line_end = None

        # Атрибут для проверки точки
        self.test_point = None

        # Атрибуты для классификации точки относительно ребра
        self.classification_edge = None
        self.classification_point = None
        self.classification_result = None

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

        ttk.Button(control_buttons_frame, text="Выбрать полигон", command=self.select_polygon_mode).pack(fill=tk.X,
                                                                                                         pady=2)
        ttk.Button(control_buttons_frame, text="Снять выделение", command=self.deselect_polygon).pack(fill=tk.X, pady=2)
        ttk.Button(control_buttons_frame, text="Очистить сцену", command=self.clear_scene).pack(fill=tk.X, pady=2)

        transform_frame = ttk.LabelFrame(control_frame, text="Аффинные преобразования", padding=10)
        transform_frame.pack(fill=tk.X, pady=5)

        ttk.Button(transform_frame, text="Смещение на dx, dy", command=self.translate_polygon).pack(fill=tk.X, pady=2)
        ttk.Button(transform_frame, text="Поворот вокруг точки", command=self.rotate_around_point).pack(fill=tk.X,
                                                                                                        pady=2)
        ttk.Button(transform_frame, text="Поворот вокруг центра", command=self.rotate_around_center).pack(fill=tk.X,
                                                                                                          pady=2)
        ttk.Button(transform_frame, text="Масштаб относительно точки", command=self.scale_around_point).pack(fill=tk.X,
                                                                                                             pady=2)
        ttk.Button(transform_frame, text="Масштаб относительно центра", command=self.scale_around_center).pack(
            fill=tk.X, pady=2)

        geometry_frame = ttk.LabelFrame(control_frame, text="Геометрические операции", padding=10)
        geometry_frame.pack(fill=tk.X, pady=5)

        ttk.Button(geometry_frame, text="Поиск пересечения ребер", command=self.find_edge_intersection).pack(fill=tk.X,
                                                                                                             pady=2)
        ttk.Button(geometry_frame, text="Проверка точки в полигоне", command=self.point_in_polygon_test).pack(fill=tk.X,
                                                                                                              pady=2)
        ttk.Button(geometry_frame, text="Классификация точки относительно ребра",
                   command=self.point_edge_classification).pack(fill=tk.X, pady=2)

        info_frame = ttk.LabelFrame(control_frame, text="Информация", padding=10)
        info_frame.pack(fill=tk.X, pady=5)

        self.info_label = ttk.Label(info_frame,
                                    text="Режим: Создание полигонов\nКликните на холсте для добавления точек",
                                    wraplength=350)
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
            if len(self.edges) == 0:
                self.intersection_point = None

            self.dynamic_line_end = None
            self.edges.append((x, y))

            if len(self.edges) == 1:
                self.update_info("Задайте вторую точку первого ребра.")
            elif len(self.edges) == 2:
                self.update_info("Задайте первую точку второго ребра.")
            elif len(self.edges) == 3:
                self.update_info("Задайте вторую точку второго ребра.")
            elif len(self.edges) == 4:
                p1, p2, p3, p4 = self.edges
                intersection = self.calculate_intersection(p1, p2, p3, p4)

                info_text = ""  # Готовим текст для инфо-панели
                if intersection:
                    self.intersection_point = intersection
                    info_text = f"Пересечение в точке: ({intersection[0]:.1f}, {intersection[1]:.1f})."
                    # messagebox.showinfo("Пересечение", f"Ребра пересекаются в точке: ({intersection[0]:.2f}, {intersection[1]:.2f})")
                else:
                    self.intersection_point = None
                    info_text = "Ребра не пересекаются."
                    # messagebox.showinfo("Пересечение", "Ребра не пересекаются.")

                self.edges = []
                self.update_info(f"{info_text}\nКликните, чтобы задать первую точку нового ребра.")

            self.draw_current_state()

        elif self.mode == "point_in_polygon":
            polygon = self.polygons[self.selected_polygon_idx]
            is_inside = self.is_point_in_polygon(x, y, polygon)

            self.test_point = (x, y, is_inside)
            self.draw_current_state()

            result_text = "ВНУТРИ" if is_inside else "СНАРУЖИ"
            # messagebox.showinfo("Результат", f"Точка ({x}, {y}) находится {result_text} полигона.")

            self.update_info(f"Точка ({x}, {y}) - {result_text}.\nКликните на следующую точку для проверки.")

        elif self.mode == "point_edge_classification":
            self.handle_point_edge_classification_click(x, y)

    def on_canvas_motion(self, event):
        self.status_var.set(f"Позиция: ({event.x}, {event.y})")
        if self.mode == "edge_intersection" and len(self.edges) in [1, 3]:
            self.dynamic_line_end = (event.x, event.y)
            self.draw_current_state()

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
        self.intersection_point = None
        self.dynamic_line_end = None
        self.classification_edge = None
        self.classification_point = None
        self.classification_result = None
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
            # Проверка на принадлежность полигону должна идти первой для больших фигур
            if self.is_point_in_polygon(x, y, polygon) or self.is_point_near_edge(x, y,
                                                                                  polygon) or self.is_point_near_vertex(
                    x, y, polygon):
                selected_idx = i
                break

        if selected_idx is not None:
            self.selected_polygon_idx = selected_idx
            self.draw_current_state()
            poly_type = self.get_polygon_type(self.polygons[selected_idx])
            self.update_info(
                f"Выбран полигон {selected_idx + 1} ({poly_type}) с {len(self.polygons[selected_idx]) - 1} вершинами")
        else:
            self.deselect_polygon()
            self.update_info("Полигон не найден")

    def is_point_near_vertex(self, x, y, polygon, threshold=10):
        for px, py in polygon:
            if math.sqrt((px - x) ** 2 + (py - y) ** 2) <= threshold:
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

        if dx == 0 and dy == 0:  # Если точки отрезка совпадают
            return math.sqrt((x - x1) ** 2 + (y - y1) ** 2)

        dot = (x - x1) * dx + (y - y1) * dy
        len_sq = dx * dx + dy * dy
        t = max(0, min(1, dot / len_sq))

        closest_x = x1 + t * dx
        closest_y = y1 + t * dy

        return math.sqrt((x - closest_x) ** 2 + (y - closest_y) ** 2)

    def is_point_in_polygon(self, x, y, polygon):
        if len(polygon) < 3:
            return False

        n = len(polygon)
        inside = False

        p1x, p1y = polygon[0]
        for i in range(1, n):
            p2x, p2y = polygon[i]
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
        # -1, так как последняя точка дублирует первую для замкнутых фигур
        num_vertices = len(polygon) - 1
        if num_vertices == 0:
            return "Точка"
        elif num_vertices == 1:
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
                # Реальная логика смещения
                polygon = self.polygons[self.selected_polygon_idx]
                self.polygons[self.selected_polygon_idx] = [(p[0] + dx, p[1] + dy) for p in polygon]
                self.draw_current_state()
                messagebox.showinfo("Смещение", f"Полигон смещен на dx={dx}, dy={dy}")
                dialog.destroy()
            except ValueError:
                messagebox.showerror("Ошибка", "Введите числовые значения")

        ttk.Button(dialog, text="Применить", command=apply_translation).pack(pady=10)

    def rotate_around_point(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("Поворот вокруг точки")
        dialog.geometry("350x200")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Введите параметры поворота:").pack(pady=10)

        # Точка вращения
        point_frame = ttk.Frame(dialog)
        point_frame.pack(pady=5)
        ttk.Label(point_frame, text="Точка (x,y):").pack(side=tk.LEFT)
        x_entry = ttk.Entry(point_frame, width=8)
        x_entry.pack(side=tk.LEFT, padx=2)
        x_entry.insert(0, "100")
        y_entry = ttk.Entry(point_frame, width=8)
        y_entry.pack(side=tk.LEFT, padx=2)
        y_entry.insert(0, "100")

        # Угол поворота
        angle_frame = ttk.Frame(dialog)
        angle_frame.pack(pady=5)
        ttk.Label(angle_frame, text="Угол (градусы):").pack(side=tk.LEFT)
        angle_entry = ttk.Entry(angle_frame, width=10)
        angle_entry.pack(side=tk.LEFT, padx=5)
        angle_entry.insert(0, "45")

        def apply_rotation():
            try:
                center_x = float(x_entry.get())
                center_y = float(y_entry.get())
                angle_deg = float(angle_entry.get())
                
                polygon = self.polygons[self.selected_polygon_idx]
                self.polygons[self.selected_polygon_idx] = self.rotate_polygon(polygon, center_x, center_y, angle_deg)
                self.draw_current_state()
                messagebox.showinfo("Поворот", f"Полигон повернут на {angle_deg}° вокруг точки ({center_x}, {center_y})")
                dialog.destroy()
            except ValueError:
                messagebox.showerror("Ошибка", "Введите числовые значения")

        ttk.Button(dialog, text="Применить", command=apply_rotation).pack(pady=10)

    def rotate_around_center(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("Поворот вокруг центра")
        dialog.geometry("300x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Введите угол поворота:").pack(pady=10)

        angle_frame = ttk.Frame(dialog)
        angle_frame.pack(pady=5)
        ttk.Label(angle_frame, text="Угол (градусы):").pack(side=tk.LEFT)
        angle_entry = ttk.Entry(angle_frame, width=10)
        angle_entry.pack(side=tk.LEFT, padx=5)
        angle_entry.insert(0, "45")

        def apply_rotation():
            try:
                angle_deg = float(angle_entry.get())
                
                polygon = self.polygons[self.selected_polygon_idx]
                center_x, center_y = self.get_polygon_center(polygon)
                self.polygons[self.selected_polygon_idx] = self.rotate_polygon(polygon, center_x, center_y, angle_deg)
                self.draw_current_state()
                messagebox.showinfo("Поворот", f"Полигон повернут на {angle_deg}° вокруг своего центра")
                dialog.destroy()
            except ValueError:
                messagebox.showerror("Ошибка", "Введите числовые значения")

        ttk.Button(dialog, text="Применить", command=apply_rotation).pack(pady=10)

    def scale_around_point(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("Масштабирование относительно точки")
        dialog.geometry("350x200")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Введите параметры масштабирования:").pack(pady=10)

        # Точка масштабирования
        point_frame = ttk.Frame(dialog)
        point_frame.pack(pady=5)
        ttk.Label(point_frame, text="Точка (x,y):").pack(side=tk.LEFT)
        x_entry = ttk.Entry(point_frame, width=8)
        x_entry.pack(side=tk.LEFT, padx=2)
        x_entry.insert(0, "100")
        y_entry = ttk.Entry(point_frame, width=8)
        y_entry.pack(side=tk.LEFT, padx=2)
        y_entry.insert(0, "100")

        # Коэффициенты масштабирования
        scale_frame = ttk.Frame(dialog)
        scale_frame.pack(pady=5)
        ttk.Label(scale_frame, text="Масштаб (sx,sy):").pack(side=tk.LEFT)
        sx_entry = ttk.Entry(scale_frame, width=8)
        sx_entry.pack(side=tk.LEFT, padx=2)
        sx_entry.insert(0, "1.5")
        sy_entry = ttk.Entry(scale_frame, width=8)
        sy_entry.pack(side=tk.LEFT, padx=2)
        sy_entry.insert(0, "1.5")

        def apply_scaling():
            try:
                center_x = float(x_entry.get())
                center_y = float(y_entry.get())
                sx = float(sx_entry.get())
                sy = float(sy_entry.get())
                
                polygon = self.polygons[self.selected_polygon_idx]
                self.polygons[self.selected_polygon_idx] = self.scale_polygon(polygon, center_x, center_y, sx, sy)
                self.draw_current_state()
                messagebox.showinfo("Масштабирование", f"Полигон масштабирован с коэффициентами ({sx}, {sy}) относительно точки ({center_x}, {center_y})")
                dialog.destroy()
            except ValueError:
                messagebox.showerror("Ошибка", "Введите числовые значения")

        ttk.Button(dialog, text="Применить", command=apply_scaling).pack(pady=10)

    def scale_around_center(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон")
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("Масштабирование относительно центра")
        dialog.geometry("300x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Введите коэффициенты масштабирования:").pack(pady=10)

        scale_frame = ttk.Frame(dialog)
        scale_frame.pack(pady=5)
        ttk.Label(scale_frame, text="Масштаб (sx,sy):").pack(side=tk.LEFT)
        sx_entry = ttk.Entry(scale_frame, width=8)
        sx_entry.pack(side=tk.LEFT, padx=2)
        sx_entry.insert(0, "1.5")
        sy_entry = ttk.Entry(scale_frame, width=8)
        sy_entry.pack(side=tk.LEFT, padx=2)
        sy_entry.insert(0, "1.5")

        def apply_scaling():
            try:
                sx = float(sx_entry.get())
                sy = float(sy_entry.get())
                
                polygon = self.polygons[self.selected_polygon_idx]
                center_x, center_y = self.get_polygon_center(polygon)
                self.polygons[self.selected_polygon_idx] = self.scale_polygon(polygon, center_x, center_y, sx, sy)
                self.draw_current_state()
                messagebox.showinfo("Масштабирование", f"Полигон масштабирован с коэффициентами ({sx}, {sy}) относительно своего центра")
                dialog.destroy()
            except ValueError:
                messagebox.showerror("Ошибка", "Введите числовые значения")

        ttk.Button(dialog, text="Применить", command=apply_scaling).pack(pady=10)

    def rotate_polygon(self, polygon, center_x, center_y, angle_deg):
        """Поворот полигона вокруг заданной точки на указанный угол"""
        angle_rad = math.radians(angle_deg)
        cos_angle = math.cos(angle_rad)
        sin_angle = math.sin(angle_rad)
        
        rotated_polygon = []
        for x, y in polygon:
            dx = x - center_x
            dy = y - center_y
            
            new_x = dx * cos_angle - dy * sin_angle
            new_y = dx * sin_angle + dy * cos_angle
            
            rotated_polygon.append((new_x + center_x, new_y + center_y))
            
        return rotated_polygon

    def scale_polygon(self, polygon, center_x, center_y, sx, sy):
        """Масштабирование полигона относительно заданной точки"""
        scaled_polygon = []
        for x, y in polygon:
            dx = x - center_x
            dy = y - center_y
            
            new_x = dx * sx
            new_y = dy * sy
            
            scaled_polygon.append((new_x + center_x, new_y + center_y))
            
        return scaled_polygon

    def get_polygon_center(self, polygon):
        """Вычисление центра полигона"""
        if len(polygon) == 0:
            return 0, 0
            
        # Для замкнутого полигона последняя точка равна первой
        points = polygon[:-1] if len(polygon) > 1 and polygon[0] == polygon[-1] else polygon
        
        x_sum = sum(p[0] for p in points)
        y_sum = sum(p[1] for p in points)
        
        return x_sum / len(points), y_sum / len(points)

    def find_edge_intersection(self):
        self.mode = "edge_intersection"
        self.edges = []
        self.intersection_point = None
        self.dynamic_line_end = None
        self.draw_current_state()
        self.update_info("Режим: Поиск пересечения ребер.\nКликните, чтобы задать первую точку первого ребра.")

    def calculate_intersection(self, p1, p2, p3, p4):
        x1, y1 = p1;
        x2, y2 = p2
        x3, y3 = p3;
        x4, y4 = p4

        den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4)

        if den == 0:
            return None

        t_num = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)
        t = t_num / den

        u_num = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3))
        u = u_num / den

        if 0 <= t <= 1 and 0 <= u <= 1:
            px = x1 + t * (x2 - x1)
            py = y1 + t * (y2 - y1)
            return (px, py)

        return None

    def point_in_polygon_test(self):
        if self.selected_polygon_idx is None:
            messagebox.showwarning("Внимание", "Сначала выберите полигон для проверки.")
            return

        if len(self.polygons[self.selected_polygon_idx]) < 3:
            messagebox.showwarning("Внимание", "Выбранный объект не является полигоном (нужно минимум 3 вершины).")
            return

        self.mode = "point_in_polygon"
        self.test_point = None
        self.draw_current_state()
        self.update_info(
            "Режим: Проверка точки в полигоне.\nКликните для проверки принадлежности точки выбранному полигону.")

    def point_edge_classification(self):
        self.mode = "point_edge_classification"
        self.classification_edge = None
        self.classification_point = None
        self.classification_result = None
        self.draw_current_state()
        self.update_info("Режим: Классификация точки относительно ребра.\nКликните для выбора двух точек ребра, затем точки для классификации.")

    def handle_point_edge_classification_click(self, x, y):
        if self.classification_edge is None:
            self.classification_edge = [(x, y)]
            self.update_info("Выбрана первая точка ребра. Кликните для выбора второй точки ребра.")
        elif len(self.classification_edge) == 1:
            self.classification_edge.append((x, y))
            self.update_info("Ребро задано. Кликните для выбора точки для классификации.")
        else:
            self.classification_point = (x, y)
            self.classification_result = self.classify_point_relative_to_edge(
                self.classification_point, 
                self.classification_edge[0], 
                self.classification_edge[1]
            )
            
            result_text = f"Точка находится {self.classification_result} относительно ребра"
            self.update_info(f"{result_text}\nКликните для нового ребра или точки.")
            
            self.classification_edge = None

        self.draw_current_state()

    def classify_point_relative_to_edge(self, point, edge_start, edge_end):
        """Классификация положения точки относительно ребра (справа или слева)"""
        x, y = point
        x1, y1 = edge_start
        x2, y2 = edge_end
        
        cross_product = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1)
        
        if cross_product > 0:
            return "СЛЕВА"
        elif cross_product < 0:
            return "СПРАВА"
        else:
            return "НА ПРЯМОЙ"

    def draw_current_state(self):
        self.canvas.delete("all")

        for i, polygon in enumerate(self.polygons):
            color = "blue"
            width = 2
            if i == self.selected_polygon_idx:
                color = "red"
                width = 3

            if len(polygon) <= 1:
                x, y = polygon[0]
                self.canvas.create_oval(x - 3, y - 3, x + 3, y + 3, fill=color, outline=color)
            elif len(polygon) == 2:
                x1, y1 = polygon[0]
                x2, y2 = polygon[1]
                self.canvas.create_line(x1, y1, x2, y2, fill=color, width=width)
            else:
                self.canvas.create_polygon(polygon[:-1], fill="lightblue", outline=color, width=width)

                # Центроид для номера полигона
                if len(polygon) > 1:
                    center_x = sum(p[0] for p in polygon[:-1]) / (len(polygon) - 1)
                    center_y = sum(p[1] for p in polygon[:-1]) / (len(polygon) - 1)
                    self.canvas.create_text(center_x, center_y, text=str(i + 1), fill="darkblue",
                                            font=("Arial", 10, "bold"))

        if self.current_polygon:
            if len(self.current_polygon) == 1:
                x, y = self.current_polygon[0]
                self.canvas.create_oval(x - 3, y - 3, x + 3, y + 3, fill="green", outline="green")
            else:
                self.canvas.create_line(self.current_polygon, fill="green", width=2)

        if self.mode == 'edge_intersection':
            if self.edges:
                for x, y in self.edges:
                    self.canvas.create_oval(x - 3, y - 3, x + 3, y + 3, fill="orange")
                if len(self.edges) >= 2:
                    self.canvas.create_line(self.edges[0], self.edges[1], fill="orange", width=3)
                if len(self.edges) == 4:
                    self.canvas.create_line(self.edges[2], self.edges[3], fill="orange", width=3)
            if self.dynamic_line_end and len(self.edges) in [1, 3]:
                start_point = self.edges[-1]
                self.canvas.create_line(start_point, self.dynamic_line_end, fill="orange", width=2, dash=(4, 2))

        if self.intersection_point:
            ix, iy = self.intersection_point
            self.canvas.create_oval(ix - 5, iy - 5, ix + 5, iy + 5, fill="red", outline="black")

        if self.test_point:
            x, y, is_inside = self.test_point
            color = "green" if is_inside else "red"
            self.canvas.create_oval(x - 5, y - 5, x + 5, y + 5, fill=color, outline="black")

        if self.mode == "point_edge_classification":
            if self.classification_edge:
                if len(self.classification_edge) >= 1:
                    x1, y1 = self.classification_edge[0]
                    self.canvas.create_oval(x1 - 4, y1 - 4, x1 + 4, y1 + 4, fill="purple", outline="purple")
                    
                    if len(self.classification_edge) >= 2:
                        x2, y2 = self.classification_edge[1]
                        self.canvas.create_oval(x2 - 4, y2 - 4, x2 + 4, y2 + 4, fill="purple", outline="purple")
                        self.canvas.create_line(x1, y1, x2, y2, fill="purple", width=3)
            
            if self.classification_point:
                x, y = self.classification_point
                color = "orange"
                if self.classification_result:
                    if "СЛЕВА" in self.classification_result:
                        color = "green"
                    elif "СПРАВА" in self.classification_result:
                        color = "red"
                    elif "НА ПРЯМОЙ" in self.classification_result:
                        color = "blue"
                
                self.canvas.create_oval(x - 5, y - 5, x + 5, y + 5, fill=color, outline="black")
                
                if self.classification_result:
                    self.canvas.create_text(x + 15, y - 15, text=self.classification_result, 
                                           fill="black", font=("Arial", 10, "bold"))

    def update_info(self, message):
        self.info_label.config(text=message)


def main():
    root = tk.Tk()
    app = PolygonEditor(root)
    root.mainloop()


if __name__ == "__main__":
    main()