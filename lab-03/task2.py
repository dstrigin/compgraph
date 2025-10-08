import tkinter as tk
import math


class LinePainter:
    def __init__(self, root):
        self.root = root
        self.root.title("Рисование отрезка")
        self.root.geometry("1000x750+200+100")
        self.root.minsize(800, 600)

        self.points = []
        self.current_algorithm = "Bresenham"

        self.create_widgets()

    def create_widgets(self):
        button_frame = tk.Frame(self.root)
        button_frame.pack(side=tk.TOP, pady=10)

        bresenham_btn = tk.Button(button_frame, text="Алгоритм Брезенхема",
                                  command=lambda: self.set_algorithm("Bresenham"))
        bresenham_btn.pack(side=tk.LEFT, padx=5)

        wu_btn = tk.Button(button_frame, text="Алгоритм Ву",
                           command=lambda: self.set_algorithm("Wu"))
        wu_btn.pack(side=tk.LEFT, padx=5)

        clear_btn = tk.Button(button_frame, text="Очистить",
                              command=self.clear_canvas)
        clear_btn.pack(side=tk.LEFT, padx=5)

        self.canvas = tk.Canvas(self.root, bg="white", width=800, height=600)
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.canvas.bind("<Button-1>", self.on_click)

    def set_algorithm(self, algorithm):
        self.current_algorithm = algorithm
        self.clear_canvas()

    def clear_canvas(self):
        self.canvas.delete("all")
        self.points = []

    def on_click(self, event):
        x, y = event.x, event.y
        self.points.append((x, y))

        self.canvas.create_oval(x - 2, y - 2, x + 2, y + 2, fill="red")

        if len(self.points) == 2:
            x1, y1 = self.points[0]
            x2, y2 = self.points[1]

            if self.current_algorithm == "Bresenham":
                self.bresenham_line(x1, y1, x2, y2)
            else:
                self.wu_line(x1, y1, x2, y2)

            self.points = []

    def bresenham_line(self, x1, y1, x2, y2):
        """Целочисленный алгоритм Брезенхема"""
        dx = abs(x2 - x1)
        dy = abs(y2 - y1)

        sx = 1 if x1 < x2 else -1
        sy = 1 if y1 < y2 else -1

        points = []

        if dx > dy:
            dy2 = 2 * dy
            dy2_minus_dx2 = 2 * (dy - dx)
            d = dy2 - dx

            x, y = x1, y1
            while x != x2 + sx:
                points.append((x, y))
                if d > 0:
                    y += sy
                    d += dy2_minus_dx2
                else:
                    d += dy2
                x += sx
        else:
            dx2 = 2 * dx
            dx2_minus_dy2 = 2 * (dx - dy)
            d = dx2 - dy

            x, y = x1, y1
            while y != y2 + sy:
                points.append((x, y))
                if d > 0:
                    x += sx
                    d += dx2_minus_dy2
                else:
                    d += dx2
                y += sy

        for x, y in points:
            self.canvas.create_rectangle(x, y, x + 1, y + 1, outline="black", fill="black")

    def wu_line(self, x0, y0, x1, y1):
        def ipart(x):
            return math.floor(x)

        def round(x):
            return ipart(x + 0.5)

        def fpart(x):
            return x - math.floor(x)

        def rfpart(x):
            return 1 - fpart(x)

        steep = abs(y1 - y0) > abs(x1 - x0)

        if steep:
            x0, y0 = y0, x0
            x1, y1 = y1, x1

        if x0 > x1:
            x0, x1 = x1, x0
            y0, y1 = y1, y0

        dx = x1 - x0
        dy = y1 - y0

        if dx == 0:
            gradient = 1.0
        else:
            gradient = dy / dx

        xend = round(x0)
        yend = y0 + gradient * (xend - x0)
        xgap = rfpart(x0 + 0.5)
        xpxl1 = xend
        ypxl1 = ipart(yend)

        if steep:
            self.plot(ypxl1, xpxl1, rfpart(yend) * xgap)
            self.plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap)
        else:
            self.plot(xpxl1, ypxl1, rfpart(yend) * xgap)
            self.plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap)

        intery = yend + gradient

        xend = round(x1)
        yend = y1 + gradient * (xend - x1)
        xgap = fpart(x1 + 0.5)
        xpxl2 = xend
        ypxl2 = ipart(yend) # вместо math.floor

        if steep:
            self.plot(ypxl2, xpxl2, rfpart(yend) * xgap)
            self.plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap)
        else:
            self.plot(xpxl2, ypxl2, rfpart(yend) * xgap)
            self.plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap)

        if steep:
            for x in range(int(xpxl1) + 1, int(xpxl2)):
                self.plot(ipart(intery), x, rfpart(intery))
                self.plot(ipart(intery) + 1, x, fpart(intery))
                intery += gradient
        else:
            for x in range(int(xpxl1) + 1, int(xpxl2)):
                self.plot(x, ipart(intery), rfpart(intery))
                self.plot(x, ipart(intery) + 1, fpart(intery))
                intery += gradient

    def plot(self, x, y, brightness):
        """Рисует пиксель с заданной яркостью (0-1)"""
        brightness = max(0, min(1, brightness))

        gray_value = int(255 * (1 - brightness))
        color = f"#{gray_value:02x}{gray_value:02x}{gray_value:02x}"

        self.canvas.create_rectangle(x, y, x + 1, y + 1, outline=color, fill=color, width=0)


if __name__ == "__main__":
    root = tk.Tk()
    app = LinePainter(root)
    root.mainloop()