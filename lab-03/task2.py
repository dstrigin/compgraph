class LinePainter:
    def __init__(self, root):
        self.root = root
        self.root.title("Рисование отрезка")
        self.root.geometry("1000x750+200+100")
        self.root.minsize(800, 600)
        