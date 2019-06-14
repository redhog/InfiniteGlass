import numpy as np

screen2glscreen = np.matrix([
    [2., 0., 0., -1.],
    [0., 2., 0., -1.],
    [0., 0., 1., 0.],
    [0., 0., 0., 1.]])

pixels = [640., 480.]
screen = [0., 0., 1., pixels[1] / pixels[0]]

def zoom_to_window(window):
    screen[2] = window[2]
    screen[3] = window[2] * pixels[1] / pixels[0]
    screen[0] = window[0]
    screen[1] = window[1]-screen[3]

def space2glscreen(space):
    space2screen = screen2glscreen.dot(np.matrix([
        [1./screen[2], 0., 0., -screen[0]/screen[2]],
        [0., 1./screen[3], 0., -screen[1]/screen[3]],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]]))
    space2screen = np.matrix([
        [1./screen[2], 0., 0., -screen[0]/screen[2]],
        [0., 1./screen[3], 0., -screen[1]/screen[3]],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]])
    res = space2screen.dot(np.matrix([[space[0]], [space[1]], [0.], [1.]]))
    return res[0,0], res[1,0]

def screen2space(screenx, screeny):
    w = pixels[0]
    h = pixels[1]
    screen2space = np.matrix([
        [screen[2]/w,0,0,screen[0]],
        [0,-screen[3]/h,0,screen[1]],
        [0,0,1,0],
        [0,0,0,1]])
    space = np.matrix([[screenx], [screeny], [0.], [1.]])
    res = screen2space.dot(space)
    return res[0,0], res[1,0]

def space2pixelscreen(spacex, spacey):
    w = pixels[0]
    h = pixels[1]
    space2screen= np.matrix([
        [w/screen[2], 0., 0., -w*screen[0]/screen[2]],
        [0., -h/screen[3], 0., -h*screen[1]/screen[3]],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]])
    space = np.matrix([[spacex], [spacey], [0.], [1.]])                          
    res = space2screen.dot(space)
    return res[0,0], res[1,0]

def draw(window):
    left = window[0]
    top = window[1]
    right = left + window[2]
    bottom = top - window[3]

    return {"left bottom": space2glscreen([left, bottom]),
            "left top": space2glscreen([left, top]),
            "right bottom": space2glscreen([right, bottom]),
            "right top": space2glscreen([right, top])}
