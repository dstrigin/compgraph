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
        """Добавить опорную точку с корректировкой для C1 гладкости."""

        # 1. Добавляем новую точку
        self.points.append((x, y))
        idx = len(self.points) - 1  # Индекс новой точки

        point_id = self.canvas.create_oval(
            x - self.point_radius, y - self.point_radius,
            x + self.point_radius, y + self.point_radius,
            fill=self.point_color, outline="black", width=2
        )
        self.point_ids.append(point_id)

        # 2. Применяем C1 гладкость, если новая точка является P_{3k+1}
        # P_{3k-1}, P_{3k}, P_{3k+1} должны быть коллинеарны

        # Проверяем, является ли новая точка P_{3k+1} (индексы 1, 4, 7, ...)
        if (idx % 3) == 1:
            knot_idx = idx - 1  # P_{3k} (узел)
            prev_partner_idx = knot_idx - 1  # P_{3k-1} (V2 предыдущего сегмента)

            # Условие: Должны существовать P_{3k} и P_{3k-1} для применения правила.
            if 0 <= prev_partner_idx < knot_idx:
                # Новая точка P_{3k+1} должна быть отражена от P_{3k-1}
                # относительно P_{3k}, что нарушило бы положение P_{3k+1}.
                # Вместо этого мы корректируем P_{3k-1} (V2) на основе новой P_{3k+1} (V1)

                knot_x, knot_y = self.points[knot_idx]

                # P_{3k-1} = 2*P_{3k} - P_{3k+1}
                partner_x = 2 * knot_x - x
                partner_y = 2 * knot_y - y

                self.points[prev_partner_idx] = (partner_x, partner_y)
                self._update_point_visual(prev_partner_idx)

        # 3. Перерисовываем
        self.redraw()

    def remove_point(self, idx: int):
        """Удалить опорную точку"""
        if 0 <= idx < len(self.points):
            self.canvas.delete(self.point_ids[idx])
            del self.points[idx]
            del self.point_ids[idx]
            self.selected_point = None
            self.redraw()

    def _update_point_visual(self, idx: int):
        """Внутренняя функция для обновления позиции маркера точки"""
        x, y = self.points[idx]
        self.canvas.coords(
            self.point_ids[idx],
            x - self.point_radius, y - self.point_radius,
            x + self.point_radius, y + self.point_radius
        )

    def move_point(self, idx: int, x: float, y: float):
        """
        Переместить опорную точку и обновить связанные точки для C1 гладкости.
        Условие: P_{3k-1}, P_{3k}, P_{3k+1} должны быть коллинеарны,
        и P_{3k} должен быть серединой отрезка P_{3k-1}P_{3k+1} (симметрия для гладкости).
        """
        if not (0 <= idx < len(self.points)):
            return

        old_x, old_y = self.points[idx]
        dx, dy = x - old_x, y - old_y

        # 1. Сначала перемещаем саму точку
        self.points[idx] = (x, y)

        # 2. Применяем правила C1 непрерывности в зависимости от типа точки (индекса)

        # Индексы 1, 4, 7, ... (P_{3k+1} - V1^k)
        if (idx % 3) == 1:
            knot_idx = idx - 1  # P_{3k}
            prev_partner_idx = knot_idx - 1  # P_{3k-1} (V2^{k-1})

            # Если существует P_{3k-1}, отражаем его относительно узла P_{3k}
            if 0 <= prev_partner_idx < len(self.points):
                knot_x, knot_y = self.points[knot_idx]

                # P_{3k-1} = 2*P_{3k} - P_{3k+1}
                partner_x = 2 * knot_x - x
                partner_y = 2 * knot_y - y

                self.points[prev_partner_idx] = (partner_x, partner_y)
                self._update_point_visual(prev_partner_idx)

        # Индексы 2, 5, 8, ... (P_{3k-1} - V2^{k-1})
        elif (idx % 3) == 2:
            knot_idx = idx + 1  # P_{3k}
            next_partner_idx = idx + 2  # P_{3k+1} (V1^k)

            # Если существует P_{3k+1}, отражаем его относительно узла P_{3k}
            if 0 <= next_partner_idx < len(self.points):
                knot_x, knot_y = self.points[knot_idx]

                # P_{3k+1} = 2*P_{3k} - P_{3k-1}
                partner_x = 2 * knot_x - x
                partner_y = 2 * knot_y - y

                self.points[next_partner_idx] = (partner_x, partner_y)
                self._update_point_visual(next_partner_idx)

        # Индексы 0, 3, 6, ... (P_{3k} - Узел/Knot)
        elif (idx % 3) == 0:
            # При перемещении узла P_{3k} его соседние контрольные точки
            # P_{3k-1} и P_{3k+1} должны сдвинуться на ту же величину (dx, dy).
            # Это сохраняет симметрию и коллинеарность.

            prev_partner_idx = idx - 1  # P_{3k-1}
            next_partner_idx = idx + 1  # P_{3k+1}

            # Сдвиг P_{3k-1}
            if 0 <= prev_partner_idx < len(self.points):
                px, py = self.points[prev_partner_idx]
                self.points[prev_partner_idx] = (px + dx, py + dy)
                self._update_point_visual(prev_partner_idx)

            # Сдвиг P_{3k+1}
            if 0 <= next_partner_idx < len(self.points):
                nx, ny = self.points[next_partner_idx]
                self.points[next_partner_idx] = (nx + dx, ny + dy)
                self._update_point_visual(next_partner_idx)

        # Обновление визуального представления текущей точки
        self._update_point_visual(idx)
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
    root = tk.Tk()  # Изменено на tk.Tk() для основного окна
    root.title("Задание 3 - Кубические сплайны Безье")
    root.geometry("800x650")

    # Заголовок
    title = tk.Label(root, text="Кубические сплайны Безье (C1 Непрерывность)", font=("Arial", 16, "bold"))
    title.pack(pady=10)

    # Инструкции
    instructions = tk.Label(
        root,
        text="ЛКМ: добавить/переместить точку | ПКМ: удалить точку\n"
             "При перемещении контрольных точек обеспечивается гладкое (C1) соединение сегментов.",
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
        # Точки: P0, P1, P2, P3 (узел), P4, P5, P6 (узел)
        demo_points = [
            (100, 250),  # P0 (Knot)
            (150, 100),  # P1
            (250, 100),  # P2 -> должен быть отражен от P4
            (300, 250),  # P3 (Knot)
            (350, 400),  # P4 -> должен быть отражен от P2
            (450, 400),  # P5
            (500, 250),  # P6 (Knot)
            (550, 150),  # P7
            (650, 150),  # P8
            (700, 250)  # P9 (Knot)
        ]
        for x, y in demo_points:
            editor.add_point(x, y)

        # Ручная коррекция для C1: P2, P3, P4 должны быть коллинеарны
        # Изначально они не коллинеарны, но после добавления все точки P_i
        # начинают работать по правилам C1, как только одну из них сдвинут.
        # Для начального гладкого состояния: P4 = 2*P3 - P2
        # P4_x = 2*300 - 250 = 350
        # P4_y = 2*250 - 100 = 400. P4 уже задан верно! (350, 400)

        # P7 = 2*P6 - P5
        # P7_x = 2*500 - 450 = 550
        # P7_y = 2*250 - 400 = 100. P7 задан как (550, 150).
        # Чтобы показать эффект, я добавлю точки, которые изначально не C1,
        # и увидим, что редактор их скорректирует при первом движении.
        # Для чистоты примера, P7 должен быть (550, 100). Исправим демо-данные.

        editor.clear()
        demo_points_c1 = [
            (100, 250), (150, 100), (250, 100), (300, 250),
            (350, 400), (450, 400), (500, 250),
            (550, 100),  # Исправлено для C1
            (650, 150), (700, 250)
        ]
        for x, y in demo_points_c1:
            editor.add_point(x, y)

    demo_btn = tk.Button(button_frame, text="Пример (Гладкий)", command=add_demo_curve, width=15)
    demo_btn.pack(side=tk.LEFT, padx=5)

    root.mainloop()