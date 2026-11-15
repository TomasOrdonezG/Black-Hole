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

vec3 getBackgroundColour(vec3 x) {
    return vec3(mod(x / 5000.0 * u_time, 1.0));
}

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

    float R = 10.0;
    float m = 1.0 - smoothstep(0.0, R, length(p - u_black_hole_pos));
    float n = fractalBrownianMotion(p * 0.4 + vec3(u_time*1.0, 0.0, 0.0)) * 0.5 + 0.5;
    return clamp(m + 0.6 * (n - 0.5) - 0.2, 0.0, 1.0);
}

vec2 raySphere(vec3 ro, vec3 rd) {
    float r = 20.0;
    vec3 oc = ro - u_black_hole_pos;
    float b = dot(oc, rd);
    float h = b * b - (dot(oc, oc) - r * r);
    if (h < 0.0) return vec2(1.0, -1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

float traceRayCloudToLight(vec3 x) {
    const float absCoeff = 0.5;
    const vec3 LIGHT_POS = vec3(50.0);

    vec3 lightDir = normalize(LIGHT_POS - x);

    // Find where the light ray exits the cloud sphere
    vec2 hit = raySphere(x, lightDir);
    float maxDist = max(hit.y, 0.0);

    // Usually you only need to march a fraction of the sphere radius
    // (e.g., 4.0 to 8.0 units) to capture deep self-shadowing
    maxDist = min(maxDist, 6.0);

    const int LIGHT_STEPS = 5;
    float stepSize = maxDist / float(LIGHT_STEPS);
    float totalDensity = 0.0;

    // Offset start position by half a step to prevent self-occlusion banding
    vec3 p = x + lightDir * (stepSize * 0.5);

    for (int i = 0; i < LIGHT_STEPS; i++) {
        float d = density(p);
        totalDensity += d * stepSize;
        p += lightDir * stepSize;
    }

    return exp(-totalDensity * absCoeff);
}

vec4 traceRayCloudTest(vec3 x, vec3 v) {
    v = normalize(v);
    vec3 bg = getBackgroundColour(x + u_max_ray_rad * v);
    bg = vec3(0.0);

    const float absCoeff = 0.5;
    const vec3 LIGHT_COLOR = vec3(1.0, 0.95, 0.9); // Sun color/intensity
    const vec3 CLOUD_ALBEDO = vec3(1.0);           // Cloud reflectivity (white)

    vec2 hit = raySphere(x, v);
    hit.x = max(hit.x, 0.0);
    if (hit.y <= hit.x) {
        return vec4(bg, 1.0);
    }

    const int STEPS = 50;
    float stepSize = (hit.y - hit.x) / float(STEPS);
    vec3 p = x + v * hit.x;

    float transmittance = 1.0;       // Starts at 1.0 (fully transparent)
    vec3 scatteredLight = vec3(0.0); // Accumulated light reaching the eye

    for (int i = 0; i < STEPS; i++) {
        float d = density(p);

        // Only compute lighting if we are actually inside cloud density
        if (d > 0.001) {
            // 1. Transmittance across this single step
            float stepTransmittance = exp(-d * absCoeff * stepSize);

            // 2. How much light reaches point p from the sun
            float lightTransmittance = traceRayCloudToLight(p);
            vec3 luminance = LIGHT_COLOR * CLOUD_ALBEDO * lightTransmittance;

            // 3. Energy-conserving integration (Sebastian Hillaire / Frostbite method)
            // Calculates the exact integral of scattered light over the step
            scatteredLight += transmittance * luminance * (1.0 - stepTransmittance);

            // 4. Attenuate total transmittance for subsequent steps behind this one
            transmittance *= stepTransmittance;

            // Early exit optimization if the cloud is now completely opaque
            if (transmittance < 0.01) {
                transmittance = 0.0;
                break;
            }
        }

        p += v * stepSize;
        if (length(p) > u_max_ray_rad && dot(p, v) > 0.0) { // Note: changed 'x' to 'p'
            break;
        }
    }

    // Blend the background sky behind the cloud with the scattered cloud light
    vec3 finalColor = bg * transmittance + scatteredLight;
    return vec4(finalColor, 1.0);
}

vec4 traceRay(vec3 x, vec3 v) {
    x -= u_black_hole_pos;

    float h = length(cross(x, v));
    float K = 1.5 * u_black_hole_r_s * h * h;
    float cK = (K > 0.0) ? u_light_speed / K : MAX_FLOAT;

    if (h < u_black_hole_cb && dot(x, v) < 0.0) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    for (int it = 0; it < u_max_ray_iters; it++) {
        float r2 = dot(x, x);
        float r = sqrt(r2);

        if (r <= u_black_hole_r_s) {
            return vec4(0.0, 0.0, 0.0, 1.0);
        }
        if (r > u_max_ray_rad && dot(x, v) > 0.0) {
            return vec4(getBackgroundColour(x), 1.0);
        }

        vec3 a = (-K / (r2 * r2 * r)) * x;
        float dt = clamp(u_dt_eps * min(r / length(v), cK * r2 * r2), u_dt_min, u_dt_max);
        v += dt * a;
        x += dt * v;
    }

    return vec4(1.0);

}

void main() {
    ivec2 windowPixel = ivec2(gl_FragCoord.xy);
    windowPixel.y = int(u_resolution.y - windowPixel.y);
    vec3 pixel = u_viewport_top_left + u_viewport_unit_x * windowPixel.x + u_viewport_unit_y * windowPixel.y;
    vec3 x = u_camera_pos;
    vec3 v = u_light_speed * normalize(pixel - x);

    // outputColor = traceRay(x, v);
    outputColor = traceRayCloudTest(x, v);
}
