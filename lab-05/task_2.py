import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, TextBox

class MidpointDisplacement:
    def __init__(self, size=129, roughness=0.7, seed=None):
        self.size = size
        self.roughness = roughness
        self.seed = seed if seed is not None else np.random.randint(0, 10000)
        np.random.seed(self.seed)
        self.height_map = np.zeros((size, size))
        self.steps_history = []
        
    def generate(self):
        self.height_map = np.zeros((self.size, self.size))
        self.height_map[0, 0] = np.random.random()
        self.height_map[0, -1] = np.random.random()
        self.height_map[-1, 0] = np.random.random()
        self.height_map[-1, -1] = np.random.random()
        
        self.steps_history = [self.height_map.copy()]
        step = self.size - 1
        
        while step > 1:
            self._diamond_step(step)
            self._square_step(step)
            step //= 2
            self.steps_history.append(self.height_map.copy())
            
        return self.height_map
    
    def _diamond_step(self, step):
        half = step // 2
        for y in range(half, self.size, step):
            for x in range(half, self.size, step):
                corners = [
                    self.height_map[y - half, x - half],
                    self.height_map[y - half, x + half],
                    self.height_map[y + half, x - half],
                    self.height_map[y + half, x + half]
                ]
                avg = np.mean(corners)
                displacement = (np.random.random() - 0.5) * step * self.roughness / self.size
                self.height_map[y, x] = avg + displacement
    
    def _square_step(self, step):
        half = step // 2
        for y in range(0, self.size, half):
            for x in range((y + half) % step, self.size, step):
                points = []
                if y >= half: points.append(self.height_map[y - half, x])
                if y + half < self.size: points.append(self.height_map[y + half, x])
                if x >= half: points.append(self.height_map[y, x - half])
                if x + half < self.size: points.append(self.height_map[y, x + half])
                
                if points:
                    avg = np.mean(points)
                    displacement = (np.random.random() - 0.5) * step * self.roughness / self.size
                    self.height_map[y, x] = avg + displacement

def main():
    generator = MidpointDisplacement()
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
    plt.subplots_adjust(bottom=0.3)
    fig.canvas.manager.set_window_title("Задание 2")
    
    height_map = generator.generate()
    
    im1 = ax1.imshow(height_map, cmap='terrain', aspect='equal')
    ax1.set_title('Финальная карта высот')
    ax1.set_xlabel('X')
    ax1.set_ylabel('Y')
    
    im2 = ax2.imshow(generator.steps_history[0], cmap='terrain', aspect='equal')
    ax2.set_title('Этап: 0')
    ax2.set_xlabel('X')
    ax2.set_ylabel('Y')
    
    ax_roughness = plt.axes([0.25, 0.2, 0.65, 0.03])
    ax_step = plt.axes([0.25, 0.15, 0.65, 0.03])
    ax_seed_slider = plt.axes([0.25, 0.1, 0.65, 0.03])
    ax_seed_text = plt.axes([0.25, 0.05, 0.2, 0.03])
    
    roughness_slider = Slider(ax_roughness, 'Шероховатость', 0.1, 2.0, 
                             valinit=generator.roughness, valstep=0.1)
    step_slider = Slider(ax_step, 'Этап', 0, len(generator.steps_history)-1, 
                        valinit=0, valfmt='%0.0f')
    seed_slider = Slider(ax_seed_slider, 'Сид (слайдер)', 0, 10000, 
                        valinit=generator.seed, valfmt='%0.0f')
    seed_textbox = TextBox(ax_seed_text, 'Сид (ввод): ', initial=str(generator.seed))
    
    def update_all():
        generator.roughness = roughness_slider.val
        generator.seed = int(seed_slider.val)
        np.random.seed(generator.seed)
        generator.generate()
        
        im1.set_data(generator.height_map)
        im1.set_clim(vmin=generator.height_map.min(), vmax=generator.height_map.max())
        
        step_idx = int(step_slider.val)
        step_slider.valmax = len(generator.steps_history) - 1
        step_data = generator.steps_history[step_idx]
        im2.set_data(step_data)
        im2.set_clim(vmin=step_data.min(), vmax=step_data.max())
        ax2.set_title(f'Этап: {step_idx}')
        
        seed_textbox.set_val(str(generator.seed))
        
        fig.canvas.draw_idle()
    
    def update_step(val):
        step_idx = int(step_slider.val)
        step_data = generator.steps_history[step_idx]
        im2.set_data(step_data)
        im2.set_clim(vmin=step_data.min(), vmax=step_data.max())
        ax2.set_title(f'Этап: {step_idx}')
        fig.canvas.draw_idle()
    
    def update_from_textbox(text):
        try:
            new_seed = int(text)
            if 0 <= new_seed <= 10000:
                seed_slider.set_val(new_seed)
                update_all()
            else:
                seed_textbox.set_val(str(int(seed_slider.val)))
        except ValueError:
            seed_textbox.set_val(str(int(seed_slider.val)))
    
    def update_from_slider(val):
        seed_textbox.set_val(str(int(val)))
        update_all()
    
    roughness_slider.on_changed(lambda val: update_all())
    seed_slider.on_changed(update_from_slider)
    seed_textbox.on_submit(update_from_textbox)
    step_slider.on_changed(update_step)
    
    plt.show()

if __name__ == "__main__":
    main()