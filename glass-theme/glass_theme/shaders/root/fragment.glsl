#version 330 core
precision highp float;

/* Copyright (c) 2020 alexkh
   This code is available as a stand-alone fractal demo
   at https://github.com/alexkh/gllyap
*/

uniform ivec2 size;
uniform vec4 root_IG_VIEW_DESKTOP_VIEW;

in vec2 px_coord;

out vec4 fragColor;

vec2 iResolution = vec2(float(size.x), float(size.y));

vec3 calc(in vec2 p) {
  float w1, w2, w3, w4, w5;
  /*
    w1 = 0.97 + 0.04*sin(0.0 + 1.3*iGlobalTime*0.2);
    w2 = 0.97 + 0.04*sin(1.0 + 1.7*iGlobalTime*0.2);
    w3 = 0.97 + 0.04*sin(4.0 + 1.1*iGlobalTime*0.2);
    w4 = 0.97 + 0.04*sin(2.0 + 1.5*iGlobalTime*0.2);
    w5 = 0.97 + 0.04*sin(5.0 + 1.9*iGlobalTime*0.2);
  */
  // w3 = w5 = 1.0;
  w1 = w2 = w3 = w4 = w5 = 1.0;

  float x = 0.5;
  float h = 0.0;
  for (int i = 0; i < FRACTAL_PRECISION; i++) {
    x = w1*p.x*x*(1.0-x); h += log2(abs(w1*p.x*(1.0-2.0*x)));
    x = w2*p.x*x*(1.0-x); h += log2(abs(w2*p.x*(1.0-2.0*x)));
    x = w3*p.y*x*(1.0-x); h += log2(abs(w3*p.y*(1.0-2.0*x)));
    x = w4*p.x*x*(1.0-x); h += log2(abs(w4*p.x*(1.0-2.0*x)));
    x = w5*p.y*x*(1.0-x); h += log2(abs(w5*p.y*(1.0-2.0*x)));
  }
  h /= 200.0 * 5.0;

  vec3 col = vec3(0.0);
  if (h < 0.0) {
    h = abs(h);
    col = vec3(0.5+0.5*sin(0.0+2.5*h),
               0.5+0.5*sin(0.4+2.5*h),
               0.5+0.5*sin(0.7+2.5*h));
    col *= vec3(1.1)*pow(h,0.25);
  }

  return col;
}

void main() {
  vec3 col;
  vec2 coord = px_coord;
  coord.y = iResolution.y - coord.y;
  
  if (BACKGROUND_TYPE == 1) {
    #if 1
      col = calc(3.0 + (1.0 * coord / iResolution.xy) * root_IG_VIEW_DESKTOP_VIEW.zw + root_IG_VIEW_DESKTOP_VIEW.xy);
    #else
      col = calc(3.0 + 1.0*(gl_FragCoord.xy+vec2(0.0,0.0)) / iResolution.xy) +
            calc(3.0 + 1.0*(gl_FragCoord.xy+vec2(0.0,0.5)) / iResolution.xy) +
            calc(3.0 + 1.0*(gl_FragCoord.xy+vec2(0.5,0.0)) / iResolution.xy) +
            calc(3.0 + 1.0*(gl_FragCoord.xy+vec2(0.5,0.5)) / iResolution.xy);
      col /= 4.0;
    #endif
  } else {
    col = vec3(1., 1., 1.);
  }
  gl_FragColor = vec4(col, 1.0);
}
