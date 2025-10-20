import tkinter as tk
from typing import List, Tuple, Optional


class BezierSplineEditor:
    def __init__(self, canvas: tk.Canvas):
        self.canvas = canvas
        self.points: List[Tuple[float, float]] = []
        self.point_ids: List[int] = []
        self.selected_point: Optional[int] = None
        self.curve_ids: List[int] = []

        # Настройки визуализации
        self.point_radius = 5
        self.point_color = "red"
        self.selected_color = "blue"
        self.curve_color = "black"
        self.control_line_color = "gray"
        self.curve_segments = 50

        # Привязка событий мыши
        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<B1-Motion>", self.on_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)
        self.canvas.bind("<Button-3>", self.on_right_click)

    def add_point(self, x: float, y: float):
        """Добавить опорную точку"""
        self.points.append((x, y))
        point_id = self.canvas.create_oval(
            x - self.point_radius, y - self.point_radius,
            x + self.point_radius, y + self.point_radius,
            fill=self.point_color, outline="black", width=2
        )
        self.point_ids.append(point_id)
        self.redraw()

    def remove_point(self, idx: int):
        """Удалить опорную точку"""
        if 0 <= idx < len(self.points):
            self.canvas.delete(self.point_ids[idx])
            del self.points[idx]
            del self.point_ids[idx]
            self.selected_point = None
            self.redraw()

    def move_point(self, idx: int, x: float, y: float):
        """Переместить опорную точку"""
        if 0 <= idx < len(self.points):
            self.points[idx] = (x, y)
            self.canvas.coords(
                self.point_ids[idx],
                x - self.point_radius, y - self.point_radius,
                x + self.point_radius, y + self.point_radius
            )
            self.redraw()

    def find_point_at(self, x: float, y: float) -> Optional[int]:
        """Найти точку по координатам"""
        for i, (px, py) in enumerate(self.points):
            if (px - x) ** 2 + (py - y) ** 2 <= (self.point_radius + 5) ** 2:
                return i
        return None

    def bezier_curve(self, p0: Tuple[float, float], p1: Tuple[float, float],
                     p2: Tuple[float, float], p3: Tuple[float, float]) -> List[Tuple[float, float]]:
        """Вычислить точки кубической кривой Безье"""
        curve_points = []
        for i in range(self.curve_segments + 1):
            t = i / self.curve_segments
            t2 = t * t
            t3 = t2 * t
            mt = 1 - t
            mt2 = mt * mt
            mt3 = mt2 * mt

            x = mt3 * p0[0] + 3 * mt2 * t * p1[0] + 3 * mt * t2 * p2[0] + t3 * p3[0]
            y = mt3 * p0[1] + 3 * mt2 * t * p1[1] + 3 * mt * t2 * p2[1] + t3 * p3[1]
            curve_points.append((x, y))
        return curve_points

    def redraw(self):
        """Перерисовать все кривые"""
        # Удалить старые кривые и линии
        for cid in self.curve_ids:
            self.canvas.delete(cid)
        self.curve_ids.clear()

        # Рисовать кривые Безье (каждые 4 точки образуют один сегмент)
        if len(self.points) >= 4:
            # Рисовать контрольные линии
            for i in range(0, len(self.points) - 1):
                line_id = self.canvas.create_line(
                    self.points[i][0], self.points[i][1],
                    self.points[i + 1][0], self.points[i + 1][1],
                    fill=self.control_line_color, dash=(2, 2), width=1
                )
                self.curve_ids.append(line_id)

            # Рисовать кубические кривые Безье
            for i in range(0, len(self.points) - 3, 3):
                p0 = self.points[i]
                p1 = self.points[i + 1]
                p2 = self.points[i + 2]
                p3 = self.points[i + 3]

                curve_points = self.bezier_curve(p0, p1, p2, p3)

                # Рисовать кривую по сегментам
                for j in range(len(curve_points) - 1):
                    line_id = self.canvas.create_line(
                        curve_points[j][0], curve_points[j][1],
                        curve_points[j + 1][0], curve_points[j + 1][1],
                        fill=self.curve_color, width=2
                    )
                    self.curve_ids.append(line_id)

        # Поднять точки на передний план
        for point_id in self.point_ids:
            self.canvas.tag_raise(point_id)

    def highlight_point(self, idx: Optional[int]):
        """Подсветить выбранную точку"""
        for i, point_id in enumerate(self.point_ids):
            if i == idx:
                self.canvas.itemconfig(point_id, fill=self.selected_color)
            else:
                self.canvas.itemconfig(point_id, fill=self.point_color)

    def on_click(self, event):
        """Обработка клика мыши"""
        idx = self.find_point_at(event.x, event.y)
        if idx is not None:
            self.selected_point = idx
            self.highlight_point(idx)
        else:
            # Добавить новую точку
            self.add_point(event.x, event.y)

    def on_drag(self, event):
        """Обработка перетаскивания"""
        if self.selected_point is not None:
            self.move_point(self.selected_point, event.x, event.y)

    def on_release(self, event):
        """Обработка отпускания кнопки мыши"""
        if self.selected_point is not None:
            self.highlight_point(None)
            self.selected_point = None

    def on_right_click(self, event):
        """Обработка правого клика (удаление точки)"""
        idx = self.find_point_at(event.x, event.y)
        if idx is not None:
            self.remove_point(idx)

    def clear(self):
        """Очистить все точки"""
        for point_id in self.point_ids:
            self.canvas.delete(point_id)
        for curve_id in self.curve_ids:
            self.canvas.delete(curve_id)
        self.points.clear()
        self.point_ids.clear()
        self.curve_ids.clear()
        self.selected_point = None


def main():
    root = tk.Toplevel()
    root.title("Задание 3 - Кубические сплайны Безье")
    root.geometry("800x650")

    # Заголовок
    title = tk.Label(root, text="Кубические сплайны Безье", font=("Arial", 16, "bold"))
    title.pack(pady=10)

    # Инструкции
    instructions = tk.Label(
        root,
        text="ЛКМ: добавить/переместить точку | ПКМ: удалить точку\n"
             "Каждые 4 точки формируют один кубический сегмент Безье",
        font=("Arial", 10),
        fg="gray"
    )
    instructions.pack(pady=5)

    # Холст для рисования
    canvas = tk.Canvas(root, width=780, height=500, bg="white", relief=tk.SUNKEN, borderwidth=2)
    canvas.pack(padx=10, pady=10)

    # Создать редактор
    editor = BezierSplineEditor(canvas)

    # Кнопки управления
    button_frame = tk.Frame(root)
    button_frame.pack(pady=5)

    clear_btn = tk.Button(button_frame, text="Очистить", command=editor.clear, width=15)
    clear_btn.pack(side=tk.LEFT, padx=5)

    def add_demo_curve():
        """Добавить демонстрационную кривую"""
        editor.clear()
        demo_points = [
            (100, 250), (150, 100), (250, 100), (300, 250),
            (300, 250), (350, 400), (450, 400), (500, 250),
            (500, 250), (550, 150), (650, 150), (700, 250)
        ]
        for x, y in demo_points:
            editor.add_point(x, y)

    demo_btn = tk.Button(button_frame, text="Пример", command=add_demo_curve, width=15)
    demo_btn.pack(side=tk.LEFT, padx=5)

    root.mainloop()


