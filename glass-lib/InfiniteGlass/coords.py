import numpy

def view_to_space(screen, size, screenx, screeny):
    screeny = screeny - size[1] # FIXME: Merge into matrix...
    screen2space = numpy.array(((screen[2] / size[0], 0, 0, screen[0]),
                                (0, -screen[3] / size[1], 0, screen[1]),
                                (0, 0, 1, 0),
                                (0, 0, 0, 1)))
    space = numpy.array((screenx, screeny, 0., 1.))
    out = screen2space.dot(space)
    return out[:2]
