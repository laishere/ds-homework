from typing import Set
import pygame
from entities import Entity
from pygame import Surface


class Scene:

    def __init__(self) -> None:
        self.entities: Set[Entity] = set()
        self.surfs = dict() # 命名surface哈希表
        self.tmp_surf = None 
        self.surf_names = set() # surface名列表
        self.entity_cnt = dict() # 实体数量哈希表

    def change(self, entity: Entity, delta):
        k = type(entity)
        if k not in self.entity_cnt:
            self.entity_cnt[k] = 1
        else:
            self.entity_cnt[k] += delta
    
    def create_elevation_for_entity(self, entity: Entity): ...

    def add_entity(self, entity: Entity):
        self.entities.add(entity)
        self.change(entity, 1)
        entity.set_elevation(self.create_elevation_for_entity(entity))

    def remove_entity(self, entity: Entity):
        self.entities.remove(entity)
        self.change(entity, -1)
    
    def get_surface(self, name, size):
        if name not in self.surf_names:
            return None
        surf = self.surfs.get(name)
        if surf is None or surf.get_size() != size:
            surf = Surface(size, pygame.SRCALPHA)
            self.surfs[name] = surf
        return surf
    
    def get_tmp_surface(self, size):
        if self.tmp_surf is None or self.tmp_surf.get_size() != size:
            self.tmp_surf = Surface(size, pygame.SRCALPHA)
        return self.tmp_surf

    def render_surfs(self): ...
    
    def render(self, surface: Surface, t):
        # 实体绘制逻辑
        size = surface.get_size()
        tmp_surf = self.get_tmp_surface(size)
        self.ctx_surface = surface
        for name in self.surf_names:
            surf = self.get_surface(name, size)
            surf.fill((0, 0, 0, 0))
        entities = list(self.entities)
        entities.sort(key=lambda e: e.elevation) # 在下面的优先绘制
        for entity in entities:
            rect = entity.update(tmp_surf, t) # 获取实体绘制区域
            tmp_surf.fill((0, 0, 0, 0), rect) # 填充透明像素
            entity.draw() # 实体绘制
            surf = self.get_surface(entity.surf_name, size) # 获取实体要渲染到的surface
            surf.blit(tmp_surf, rect, rect) # 把临时surface上的内容渲染到surface上（无法直接在tmp_surf上绘制，因为直接绘制无法正确处理透明度信息，必须blit）
        self.render_surfs() # 渲染到传入的surface上
        dead = []
        for e in self.entities: 
            if not e.isAlive:
                dead.append(e) # 添加已经过期的实体
        for e in dead: self.remove_entity(e) # 删除这些过期实体
