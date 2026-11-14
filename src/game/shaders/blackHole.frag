#version 150

const float MAX_FLOAT = 3.402823466e+38;

// Game context
uniform vec2  u_resolution;
uniform float u_time;

// Camera
uniform vec3 u_camera_pos;
uniform vec3 u_viewport_unit_x;
uniform vec3 u_viewport_unit_y;
uniform vec3 u_viewport_top_left;

// Constants
uniform float u_light_speed;
uniform float u_grav_const;

// Settings
uniform int u_max_ray_iters;
uniform float u_max_ray_rad;
uniform float u_dt_eps;
uniform float u_dt_min;
uniform float u_dt_max;

// Black hole
uniform vec3 u_black_hole_pos;
uniform float u_black_hole_mass;
uniform float u_black_hole_r_s;
uniform float u_black_hole_cb;

// Clouds
uniform sampler3D u_noise;

out vec4 outputColor;

float fractalBrownianMotion(vec3 p) {
    float sum = 0.0;
    float amp = 0.5;
    float norm = 0.0;

    for (int i = 0; i < 5; i++) {
        sum += amp * texture(u_noise, p).r;
        norm += amp;
        p = p * 2.02 + vec3(0.13, 0.31, 0.07);
        amp *= 0.5;
    }
    return sum / norm;
}

float density(vec3 p)
{
    // shape mask: 1 at the center, 0 at radius R, smooth in between
    float R = 2.0;
    float m = 1.0 - smoothstep(0.0, R, length(p - vec3(0.0, 0.0, 6.0)));

    // noise in [0,1]
    float n = fractalBrownianMotion(p * 0.4 + vec3(u_time * 0.02, 0.0, 0.0)) * 0.5 + 0.5;

    // additive style: mask defines the shape, noise roughens it
    return clamp(m + 0.6 * (n - 0.5) - 0.2, 0.0, 1.0);
}

vec3 traceRay(vec3 x, vec3 v) {
    x -= u_black_hole_pos;

    float h = length(cross(x, v));
    float K = 1.5 * u_black_hole_r_s * h * h;
    float cK = (K > 0.0) ? u_light_speed / K : MAX_FLOAT;

    if (h < u_black_hole_cb && dot(x, v) < 0.0) {
        return vec3(0.0);
    }

    for (int it = 0; it < u_max_ray_iters; it++) {
        float r2 = dot(x, x);
        float r = sqrt(r2);

        if (r <= u_black_hole_r_s) {
            return vec3(0.0);
        }
        if (r > u_max_ray_rad && dot(x, v) > 0.0) {
            return vec3(mod(x / 5000.0 * u_time, 1.0));
            vec2 uv = gl_FragCoord.xy / u_resolution;
            return 0.5 + 0.5 * cos(u_time + uv.xyx + vec3(0.0, 2.0, 4.0));
        }

        vec3 a = (-K / (r2 * r2 * r)) * x;
        float dt = clamp(u_dt_eps * min(r / length(v), cK * r2 * r2), u_dt_min, u_dt_max);
        v += dt * a;
        x += dt * v;
    }

    return vec3(1.0);

}

void main() {
    ivec2 windowPixel = ivec2(gl_FragCoord.xy);
    vec3 pixel = u_viewport_top_left + u_viewport_unit_x * windowPixel.x + u_viewport_unit_y * windowPixel.y;
    vec3 x = u_camera_pos;
    vec3 v = u_light_speed * normalize(pixel - x);

    outputColor = vec4(traceRay(x, v), 1.0);
}
