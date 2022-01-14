import pygame
from entities import Rain, Entity, Ripple, clamp
from scene import Scene
from pygame import Surface
import cv2
import numpy as np
from numpy.linalg import inv

class Pond(Scene):
    SURF_RAIN = 'rain'
    SURF_RIPPLE = 'ripple'

    def __init__(self) -> None:
        super().__init__()
        self.surf_names.add(Pond.SURF_RAIN)
        self.surf_names.add(Pond.SURF_RIPPLE)
        self.ripple_transfrom_ang = np.pi * 0.1 # 旋转水面的角度（相对于视平面）
        self.ripple_far_ratio = 0.5 # 透视远端的大小比例
        self.rain_cnt = 30 # 雨滴数量
        self.ctx_size = (0, 0)
        self.entity_elevation = 0
        self.wind_spd = 0 # 风速
        self.rain_spd = 500 # 雨滴速度
        self.rain_sound = pygame.mixer.Sound('rain.mp3') # 雨声
        self.rain_hits = [0] * 100 # 雨滴入水记录循环队列数组
        self.rain_hit_index = 0 # 队尾下标
        self.rain_hits_sum = 0 # 最近这段时间的雨滴入水数
        self.rain_sound.set_volume(0)
        self.rain_sound.play(-1)

        for _ in range(self.rain_cnt):
            self.add_entity(Rain()) # 增加雨滴

    def change_wind(self, delta):
        max_spd = 100
        self.wind_spd = clamp(self.wind_spd + delta, -max_spd, max_spd) # 更改场景风速

    def change_rain_spd(self, delta):
        a, b = 100, 1000
        self.rain_spd = clamp(self.rain_spd + delta, a, b)
        self.rain_cnt = int((self.rain_spd - a) / (b - a) * 50 + 5) # 更场景雨滴速度

    def add_entity(self, entity: Entity):
        super().add_entity(entity)
        if type(entity) is Rain:
            entity.set_surf_name(Pond.SURF_RAIN) # 为雨滴设置绘制surface
        elif type(entity) is Ripple:
            entity.set_surf_name(Pond.SURF_RIPPLE) # 为水波设置绘制surface

    def create_elevation_for_entity(self, entity: Entity):
        self.entity_elevation += 1
        return self.entity_elevation # 返回下一个实体的层叠高度

    def check_size(self):
        if self.ctx_size != self.ctx_surface.get_size():
            self.ctx_size = self.ctx_surface.get_size()
            self.create_cv2_buf() # 初始化透视变换需要用到的buffer
            self.calc_transform() # 计算透视矩阵
            self.create_ripple_wrapper() # 初始化水波容器surface

    def create_ripple_wrapper(self):
        self.ripple_wrapper = Surface(self.ctx_size)

    def create_cv2_buf(self):
        w, h = self.ctx_size
        self.cv2_buf1 = np.ndarray((h, w, 3), dtype=np.uint8)
        self.cv2_buf2 = np.ndarray((h, w, 3), dtype=np.uint8)
        self.cv2_buf3 = np.ndarray((w, h, 3), dtype=np.uint8)

    def calc_transform(self):
        w, h = self.ctx_size
        far_ratio = self.ripple_far_ratio
        ang = self.ripple_transfrom_ang
        src = np.float32((
            (0, h),
            (w, h),
            (0, 0),
            (w, 0)
        ))
        dst = np.float32((
            (0, h),
            (w, h),
            ((1 - far_ratio) / 2 * w, (1 - np.sin(ang)) * h),
            ((1 + far_ratio) / 2 * w, (1 - np.sin(ang)) * h)
        ))
        self.cv2_mat = cv2.getPerspectiveTransform(src, dst)
        self.cv2_inv_mat = inv(self.cv2_mat) # 透视逆矩阵

    def record_rain_hit(self, cnt):
        size = len(self.rain_hits)
        prev = (self.rain_hit_index + 1) % size # 队头
        self.rain_hits_sum += cnt - self.rain_hits[prev] # 加上新的入水数量，减去过期的队头上的入水数量
        self.rain_hits[self.rain_hit_index] = cnt # 新记录存到队尾
        self.rain_hit_index = (self.rain_hit_index + 1) % size # 队尾往前挪
        self.rain_sound.set_volume(self.rain_hits_sum * 0.008 + 0.1) # 设置音量
        # print(self.rain_sound.get_volume())

    def render(self, surface: Surface, t):
        for e in self.entities:
            if type(e) is Rain:
                e.win_spd = self.wind_spd
                e.rain_spd = self.rain_spd
        return super().render(surface, t)

    def render_surfs(self):
        self.check_size()
        size = self.ctx_size
        surface = self.ctx_surface
        ripple_surf = self.surfs[Pond.SURF_RIPPLE]
        rain_surf = self.surfs[Pond.SURF_RAIN]
        cv_surf = self.ripple_wrapper
        cv_surf.fill((203, 240, 255)) # 填充池塘水面颜色
        cv_surf.blit(ripple_surf, (0, 0)) # 填充绘制好的水波
        src = pygame.surfarray.pixels3d(cv_surf) # 获取像素数据
        cv2.transpose(src, self.cv2_buf1) # pygame是(col, row) 型的，而cv2是 (row, col)型的，需要倒置
        cv2.warpPerspective(self.cv2_buf1, self.cv2_mat, size, self.cv2_buf2, borderMode=cv2.BORDER_CONSTANT, borderValue=(255, 255, 255)) # 然而cv2的size确是 w, h型的（row，col）。。。
        cv2.transpose(self.cv2_buf2, self.cv2_buf3) # 倒置回来
        del src
        pygame.surfarray.blit_array(cv_surf, self.cv2_buf3) # OpenCV处理得到的像素数据直接填充到水波容器surface
        surface.blit(cv_surf, (0, 0)) # 先绘制水面
        surface.blit(rain_surf, (0, 0)) # 再绘制雨滴
        
        w, h = size
        new_ripples = []
        hits = 0
        for entity in self.entities:
            if type(entity) is Rain:
                # if entity.isFreeze: continue
                disappear_h = np.sin(self.ripple_transfrom_ang) * entity.z
                disappear_y = h - disappear_h
                if entity.pos2[1] > disappear_y: # 入水条件
                    entity.isAlive = False # 标志为待删除实体
                    rx = entity.pos2[0] # 雨滴x
                    ry = entity.pos2[1] # 雨滴y
                    a = np.float32([[[rx, ry]]])
                    b = cv2.perspectiveTransform(a, self.cv2_inv_mat) # 逆透视
                    x, y = b[0][0]
                    hits += 1 # 入水数 +1
                    new_ripples.append(Ripple((x, y), entity.color)) # 需要新增水波

        self.record_rain_hit(hits) # 记录入水数并调整雨声音量
        
        for e in new_ripples:
            self.add_entity(e) # 加入新水波

        gen = self.rain_cnt - self.entity_cnt[Rain]
        for _ in range(max(0, gen)):
            self.add_entity(Rain()) # 需要补的雨滴


        

