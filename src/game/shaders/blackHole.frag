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

const float ABSORPTION_COEFF = 0.4;
const float SCATTERING_COEFF = 0.1;
const float EXTINCTION_COEFF = ABSORPTION_COEFF + SCATTERING_COEFF;

const float ACCRETION_DISC_THICKNESS = 20.0;
const float ACCRETION_DISC_EDGE_GRADIENT_THICKNESS = 5.0;
const float ACCRETION_DISC_HEIGHT = 1.0;
const vec3 ACCRETION_DISC_NORMAL = vec3(0.15, 1.0, 0.0);
const float ACCRETION_DISC_CLOUD_VERTICAL_THRESHOLD_FALLOFF_FACTOR = 0.15;
const float ACCRETION_DISC_CLOUD_MIN_LOWER_TRESHOLD = 0.3;
const float ACCRETION_DISC_CLOUD_MIN_UPPER_TRESHOLD = 0.6;
const float ACCRETION_DISC_ROTATION_SPEED = 45.0;
const float ACCRETION_DISC_ROTATION_PERIOD = 100.0;

const vec3 LIGHT_POS = vec3(10.0);
const vec3 LIGHT_COLOR = vec3(1.0, 0.95, 0.9);

const int CLOUD_RAYMARCH_STEPS = 100;
const int CLOUD_RAYMARCH_STEPS_TO_LIGHT = 10;
const vec3 CLOUD_ALBEDO = vec3(1.0);
const float CLOUD_SCALE = 0.2;

const float LEVEL_OF_DETAIL = 1.0;

out vec4 outputColor;

vec3 getBackgroundColour(vec3 x) {
    return vec3(mod(x / 5000.0 * u_time, 1.0));
}

// ----- Accretion Disc -----

float fbm(vec3 p, float lod) {
    float sum = 0.0;
    float amp = 0.5;
    float norm = 0.0;

    for (int i = 0; i < 4; i++) {
        sum += amp * textureLod(u_noise, p, lod).r;
        norm += amp;
        p *= 2.02 + vec3(0.13, 0.31, 0.07);
        amp *= 0.5;
    }
    return sum / norm;
}

float omega(float r) {
    return sqrt(u_black_hole_r_s / (2 * r * r * r));
}

float noisePhase(vec3 planePos, float r, float z, float t, float lod) {
    // Rotate according to time
    float theta = -omega(r) * t;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    vec3 rotPlanePos = planePos * cosTheta + cross(ACCRETION_DISC_NORMAL, planePos) * sinTheta;

    // Scale and domain warp the projected position
    vec3 samplePos = CLOUD_SCALE * (rotPlanePos + z * ACCRETION_DISC_NORMAL);
    samplePos += 0.6 * (vec3(
        textureLod(u_noise, samplePos * 0.5, lod).r,
        textureLod(u_noise, samplePos * 0.5 + 0.37, lod).r,
        textureLod(u_noise, samplePos * 0.5 + 0.71, lod).r
        ) - 0.5);
    return fbm(samplePos, lod);
}

float sampleCloudDensity(vec3 pos, float r, float z, float zn, float lod) {
    // Project pos onto disc
    vec3 planePos = pos - z * ACCRETION_DISC_NORMAL;

    // Overlay two periodic cloud rotations for one continuous rotation
    float periodTimePercent = fract(ACCRETION_DISC_ROTATION_SPEED * u_time / ACCRETION_DISC_ROTATION_PERIOD);
    float t1 = ACCRETION_DISC_ROTATION_PERIOD * periodTimePercent;
    float t2 = ACCRETION_DISC_ROTATION_PERIOD * fract(periodTimePercent + 0.5);
    float noise1 = noisePhase(planePos, r, z, t1, lod);
    float noise2 = noisePhase(planePos, r, z, t2, lod);
    float alpha = smoothstep(0.0, 1.0, abs(2.0 * periodTimePercent - 1.0));;
    float noise = mix(noise1, noise2, alpha);

    // Apply vertical fall off
    float kzn = ACCRETION_DISC_CLOUD_VERTICAL_THRESHOLD_FALLOFF_FACTOR * abs(zn);
    return smoothstep(ACCRETION_DISC_CLOUD_MIN_LOWER_TRESHOLD + kzn, ACCRETION_DISC_CLOUD_MIN_UPPER_TRESHOLD + kzn, noise);
}

float sampleAccretionDiscDensity(vec3 pos, float lod) {
    float z = dot(pos, ACCRETION_DISC_NORMAL);
    float zn = z / ACCRETION_DISC_HEIGHT;
    float r = sqrt(max(dot(pos, pos) - z * z, 0.0));

    float r_in = 3.0 * u_black_hole_r_s;
    float r_out = r_in + ACCRETION_DISC_THICKNESS;
    if (r < r_in || r > r_out) return 0.0;

    float verticalDensity = exp(- zn * zn / 2.0);
    if (verticalDensity < 0.01) return 0.0;

    float innerEdgeFade = smoothstep(r_in, r_in + ACCRETION_DISC_EDGE_GRADIENT_THICKNESS, r);
    float outerEdgeFade = 1.0 - smoothstep(r_out - ACCRETION_DISC_EDGE_GRADIENT_THICKNESS, r_out, r);
    float radial = pow(r / r_in, -1.5);

    float cloud = sampleCloudDensity(pos, r, z, zn, lod);
    return innerEdgeFade * outerEdgeFade * radial * verticalDensity * cloud;
}

// ----- Ray Marching ---

vec2 rayAccretionDistBoundingSphere(vec3 rayOrigin, vec3 rayDir) {
    float r_in = 3.0 * u_black_hole_r_s;
    float r = r_in + ACCRETION_DISC_THICKNESS;

    float b = dot(rayOrigin, rayDir);
    float h = b * b - (dot(rayOrigin, rayOrigin) - r * r);
    if (h < 0.0) return vec2(1.0, -1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

float cloudLightTransmittance(vec3 pos) {
    vec3 lightDir = normalize(LIGHT_POS - pos);

    vec2 intersection = rayAccretionDistBoundingSphere(pos, lightDir);
    float maxDist = max(intersection.y, 0.0);
    float stepSize = maxDist / float(CLOUD_RAYMARCH_STEPS_TO_LIGHT);

    float totalDensity = 0.0;
    for (int i = 0; i < CLOUD_RAYMARCH_STEPS_TO_LIGHT; i++) {
        float density = sampleAccretionDiscDensity(pos, 1.0);
        totalDensity += density * stepSize;
        pos += lightDir * stepSize;
    }

    return exp(-totalDensity * EXTINCTION_COEFF);
}

vec3 castRayCloud(vec3 rayOrigin, vec3 rayDir) {
    rayDir = normalize(rayDir);
    vec3 bgColour = getBackgroundColour(rayOrigin + u_max_ray_rad * rayDir);
    bgColour = vec3(0.0);

    // Check intersection with cloud bounding sphere
    vec2 intersection = rayAccretionDistBoundingSphere(rayOrigin, rayDir);
    float t1 = intersection.x;
    float t2 = intersection.y;
    if (t2 <= t1) return bgColour;

    // Compute step size
    float dist = t2 - t1;
    float stepSize  = dist / float(CLOUD_RAYMARCH_STEPS);

    // Step through cloud
    float transmittance = 1.0;
    float lightEnergy = 0.0;
    for (int i = 0; i < CLOUD_RAYMARCH_STEPS; i++) {
        float rayLength = t1 + i * stepSize;
        vec3 pos = rayOrigin + rayLength * rayDir;
        float density = sampleAccretionDiscDensity(pos, LEVEL_OF_DETAIL);
        if (density < 0.001) continue;

        // Update light energy and transmittance
        float lightTransmittance = cloudLightTransmittance(pos);
        float stepTransmittance = exp(-density * EXTINCTION_COEFF * stepSize);
        lightEnergy += transmittance * (1.0 - stepTransmittance) * lightTransmittance;
        transmittance *= stepTransmittance;

        if (transmittance < 0.01) break;
    }

    return bgColour * transmittance + lightEnergy * LIGHT_COLOR;
}

vec3 castRay(vec3 x, vec3 v) {
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
            return getBackgroundColour(x);
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
    windowPixel.y = int(u_resolution.y - windowPixel.y);
    vec3 pixel = u_viewport_top_left + u_viewport_unit_x * windowPixel.x + u_viewport_unit_y * windowPixel.y;
    vec3 x = u_camera_pos;
    vec3 v = u_light_speed * normalize(pixel - x);

    // outputColor = vec4(traceRay(x, v), 1.0);
    outputColor = vec4(castRayCloud(x, v), 1.0);
}
