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

const float EXPOSURE = 1.0;

const float ABSORPTION_COEFF = 0.5;
const float SCATTERING_COEFF = 0.2;
const float EXTINCTION_COEFF = ABSORPTION_COEFF + SCATTERING_COEFF;

const float ACCRETION_DISC_THICKNESS = 30.0;
const float ACCRETION_DISC_EDGE_GRADIENT_THICKNESS = 7.5;
const float ACCRETION_DISC_HEIGHT = 0.6;
const vec3 ACCRETION_DISC_NORMAL = vec3(0.05, 1.0, 0.0);
const float ACCRETION_DISC_CLOUD_VERTICAL_THRESHOLD_FALLOFF_FACTOR = 0.15;
const float ACCRETION_DISC_CLOUD_MIN_LOWER_TRESHOLD = 0.3;
const float ACCRETION_DISC_CLOUD_MIN_UPPER_TRESHOLD = 0.75;
const float ACCRETION_DISC_ROTATION_SPEED = 10.0;
const float ACCRETION_DISC_ROTATION_PERIOD = 100.0;
const float ACCRETION_DISC_MAX_TEMPERATURE = 3500.0;

const float ACCRETION_DISC_NOISE_SCALE = 0.15;

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
    vec3 samplePos = ACCRETION_DISC_NOISE_SCALE * (rotPlanePos + z * ACCRETION_DISC_NORMAL);
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

vec2 rayAccretionDiscBoundingSphere(vec3 rayOrigin, vec3 rayDir) {
    float r_in = 3.0 * u_black_hole_r_s;
    float r = r_in + ACCRETION_DISC_THICKNESS;

    float b = dot(rayOrigin, rayDir);
    float h = b * b - (dot(rayOrigin, rayOrigin) - r * r);
    if (h < 0.0) return vec2(1.0, -1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

/**
 * Converts a temperature to an RGB colour.
 */
vec3 tannerHellandLinear(float temp) {
    temp /= 100.0;
    vec3 c = vec3(0.0);

    // Calculate red
    if (temp <= 66.0) {
        c.r = 255.0;
    } else {
        c.r = clamp(329.698727446 * pow(temp - 60.0, -0.1332047592), 0.0, 255.0);
    }

    // Calculate green
    if (temp <= 66.0) {
        c.g = clamp(99.4708025861 * log(temp) - 161.1195681661, 0.0, 255.0);
    } else {
        c.g = clamp(288.1221695283 * pow(temp - 60.0, -0.0755148492), 0.0, 255.0);
    }

    // Calculate blue
    if (temp >= 66.0) {
        c.b = 255.0;
    } else if (temp <= 19.0) {
        c.b = 0.0;
    } else {
        c.b = clamp(138.5177312231 * log(temp - 10.0) - 305.0447927307, 0.0, 255.0);
    }

    // Normalize and convert to linear space
    return pow(c / 255.0, vec3(2.2));
}

vec3 accretionDiscColour(vec3 pos, vec3 unitDir) {
    float r = length(pos);
    float r_in = 3.0 * u_black_hole_r_s;
    if (r < r_in) return vec3(0.0);

    // Shift factor
    float orbitalSpeed2 = u_black_hole_r_s / (2.0 * (r - u_black_hole_r_s));
    float num = sqrt((1.0 - u_black_hole_r_s / r) * (1.0 - orbitalSpeed2));
    vec3 unitVel = normalize(cross(ACCRETION_DISC_NORMAL, pos));
    float denom = 1.0 - sqrt(orbitalSpeed2) * dot(unitVel, -unitDir);
    float shiftFactor = num / denom;

    // Compute temperature and apply shift
    float r_in_r = r_in / r;
    float tempProfile = pow(r_in_r, 0.75) * pow(max(0.0, 1.0 - sqrt(r_in_r)), 0.25);
    float tempEmit = ACCRETION_DISC_MAX_TEMPERATURE * 2.05 * tempProfile;
    float temp = shiftFactor * tempEmit;

    // Apply intensity
    float intensity = pow(shiftFactor, 4.0);
    vec3 col = intensity * tannerHellandLinear(temp);

    return col;
}

vec3 castRayCloud(vec3 rayOrigin, vec3 unitRayDir) {
    rayOrigin -= u_black_hole_pos;
    vec3 bgColour = vec3(0.0);

    // Constants for ray marching
    float h = length(cross(rayOrigin, unitRayDir));
    float K = 1.5 * u_black_hole_r_s * h * h;
    float cK = (K > 0.0) ? u_light_speed / K : MAX_FLOAT;

    float transmittance = 1.0;
    vec3 light = vec3(0.0);
    vec3 rayDir = u_light_speed * unitRayDir;

    for (int it = 0; it < u_max_ray_iters; it++) {
        float r2 = dot(rayOrigin, rayOrigin);
        float r = sqrt(r2);

        if (r <= u_black_hole_r_s || (r > u_max_ray_rad && dot(rayOrigin, rayDir) > 0.0)) break;

        float stepSize = clamp(u_dt_eps * min(r / length(rayDir), cK * r2 * r2), u_dt_min, u_dt_max);

        // Update light energy and transmittance
        float density = sampleAccretionDiscDensity(rayOrigin, LEVEL_OF_DETAIL);
        if (density >= 0.001) {
            unitRayDir = normalize(rayDir);
            vec3 lightTransmittance = accretionDiscColour(rayOrigin, unitRayDir);
            float actualStepDist = stepSize * length(rayDir);
            float stepTransmittance = exp(-density * EXTINCTION_COEFF * actualStepDist);
            light += transmittance * (1.0 - stepTransmittance) * lightTransmittance;
            transmittance *= stepTransmittance;
        }

        if (transmittance < 0.01) break;

        // Update ray
        vec3 rayAcc = (-K / (r2 * r2 * r)) * rayOrigin;
        rayDir += stepSize * rayAcc;
        rayOrigin += stepSize * rayDir;
    }

    return bgColour * transmittance + light;
}

// ----- Black Hole Ray Marching -----

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

vec3 acesToneMap(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    // Compute ray position and direction vectors
    ivec2 windowPixel = ivec2(gl_FragCoord.xy);
    windowPixel.y = int(u_resolution.y - windowPixel.y);
    vec3 pixel = u_viewport_top_left + u_viewport_unit_x * windowPixel.x + u_viewport_unit_y * windowPixel.y;
    vec3 rayPos = u_camera_pos;
    vec3 unitRayDir = normalize(pixel - rayPos);

    // Cast ray to compute colour
    vec3 hdrColour = castRayCloud(rayPos, unitRayDir);

    // Apply exposure, tone map, and gamma correction to final colour
    hdrColour *= EXPOSURE;
    vec3 mapped = acesToneMap(hdrColour);
    vec3 gammaCorrected = pow(mapped, vec3(1.0 / 2.2));

    // outputColor = vec4(hdrColour, 1.0);
    outputColor = vec4(gammaCorrected, 1.0);
}
