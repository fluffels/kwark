# Kwark
This project is a basic `.bsp` file loader / viewer.

## Motivation
I've written a few renderers for outside scenes, but wanted to try writing one for internal scenes.
Using the models and textures stored in Quake's `pak0` is an easy way to get models, textures and other resources to test the renderer with.

## Current Progress
![](screenshot.png)

- [X] Level models
- [X] Textures
- [X] Light maps
- [ ] Texture animation
- [ ] Sprites
- [ ] Monster models
- [ ] View models (i.e. weapons the player may be holding)
- [ ] Particle systems
- [ ] Level animation (triggers)

It's not clear how many of these I will end up doing.
Some of these are not straight forward to reverse engineer from the descriptions of the BSP format online.
Reading Quake source to figure out how to handle them seems like a waste of time, since I could just use that code directly.
