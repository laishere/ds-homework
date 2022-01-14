import pygame as pg
from pond import Pond

size = [1280, 720] # 分辨率

pg.init()
pg.mixer.init()
screen = pg.display.set_mode(size)

def time(): return pg.time.get_ticks() / 1000

running = True

pond = Pond() # 新建池塘场景

while running:
    # 主循环
    for event in pg.event.get():
        if event.type == pg.QUIT:
            running = False
    keys = pg.key.get_pressed()
    if keys[pg.K_RIGHT]:
        pond.change_wind(2) # 修改风速 +2
    if keys[pg.K_LEFT]:
        pond.change_wind(-2) # 修改风速 -2
    if keys[pg.K_UP]:
        pond.change_rain_spd(-10) # 修改降雨速度 -10
    if keys[pg.K_DOWN]:
        pond.change_rain_spd(10)  # 修改降雨速度 +10

    screen.fill((255, 255, 255))
    pond.render(screen, time()) # 渲染场景
    pg.display.flip() # 更新画面

pg.quit()