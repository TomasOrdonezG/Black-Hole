#version 150

const float MAX_FLOAT = 3.402823466e+38;

// Game context
uniform vec2  u_resolution;
uniform float u_time;

// Clouds
uniform sampler3D u_noise;

// Universal Physics
uniform float u_light_speed;
uniform float u_grav_const;

// Black hole
uniform vec3 u_black_hole_pos;
uniform float u_black_hole_mass;
uniform float u_black_hole_r_s;

// Accretion Disc
uniform float u_accretion_disc_thickness;
uniform float u_accretion_dist_edge_thickness_falloff_percent;
uniform float u_accretion_disc_height;
uniform float u_accretion_disc_vertical_threshold_falloff_factor;
uniform float u_accretion_disc_cloud_min_lower_threshold;
uniform float u_accretion_disc_cloud_min_upper_threshold;
uniform float u_accretion_disc_rotation_speed;
uniform float u_accretion_disc_rotation_period;
uniform float u_accretion_disc_max_temperature;
uniform float u_accretion_dist_extinction_coeff;
uniform float u_accretion_disc_cloud_size;
uniform float u_accretion_disc_swirl;
uniform vec3 u_accretion_disc_normal;

// Ray Marching
uniform bool u_rk4;
uniform int u_ray_marching_max_steps;
uniform float u_ray_marching_max_dist;
uniform float u_ray_marching_step_size_factor;
uniform float u_ray_marching_min_step_size;
uniform float u_ray_marching_max_step_size;

// Rendering
uniform float u_exposure;
uniform float u_level_of_detail;

// Camera
uniform vec3 u_camera_pos;
uniform vec3 u_viewport_unit_x;
uniform vec3 u_viewport_unit_y;
uniform vec3 u_viewport_top_left;

out vec4 outputColor;

// ----- Colour Functions -----

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

vec3 acesToneMap(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// ----- Accretion Disc -----

const mat3 ROT = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64);

float fbm(vec3 p, float lod) {
    float sum = 0.0;
    float amp = 0.5;
    float norm = 0.0;

    for (int i = 0; i < 4; i++) {
        sum += amp * textureLod(u_noise, p + float(i) * vec3(0.37, 0.71, 0.19), lod).r;
        norm += amp;
        p = ROT * p * 2.03;
        amp *= 0.5;
    }
    return sum / norm;
}

float omega(float r) {
    return sqrt(u_black_hole_r_s / (2 * r * r * r));
}

float noisePhase(vec3 planePos, float r, float z, float t, float lod) {
    // Rotate according to time
    float r_in = 3.0 * u_black_hole_r_s;
    float theta = -omega(r) * t + u_accretion_disc_swirl * log(r / r_in);
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    vec3 rotPlanePos = planePos * cosTheta + cross(u_accretion_disc_normal, planePos) * sinTheta;

    // Scale and domain warp the projected position
    vec3 samplePos = u_accretion_disc_cloud_size * (rotPlanePos + z * u_accretion_disc_normal);
    samplePos += 0.6 * (vec3(
        textureLod(u_noise, samplePos * 0.5, lod).r,
        textureLod(u_noise, samplePos * 0.5 + 0.37, lod).r,
        textureLod(u_noise, samplePos * 0.5 + 0.71, lod).r
        ) - 0.5);
    float large = textureLod(u_noise, samplePos * 0.11 + 0.5, lod).r;
    return fbm(samplePos, lod) + 0.35 * (large - 0.5);
    // return fbm(samplePos, lod);
}

float sampleCloudDensity(vec3 pos, float r, float z, float zn, float lod) {
    // Project pos onto disc
    vec3 planePos = pos - z * u_accretion_disc_normal;

    // Overlay two periodic cloud rotations for one continuous rotation
    float periodTimePercent = fract(u_accretion_disc_rotation_speed * u_time / u_accretion_disc_rotation_period);
    float t1 = u_accretion_disc_rotation_period * periodTimePercent;
    float t2 = u_accretion_disc_rotation_period * fract(periodTimePercent + 0.5);
    float noise1 = noisePhase(planePos, r, z, t1, lod);
    float noise2 = noisePhase(planePos, r, z, t2, lod);
    float alpha = smoothstep(0.0, 1.0, abs(2.0 * periodTimePercent - 1.0));;
    float noise = mix(noise1, noise2, alpha);

    // Apply vertical fall off
    float kzn = u_accretion_disc_vertical_threshold_falloff_factor * abs(zn);
    return smoothstep(u_accretion_disc_cloud_min_lower_threshold + kzn, u_accretion_disc_cloud_min_upper_threshold + kzn, noise);
}

float sampleAccretionDiscDensity(vec3 pos, float lod) {
    float z = dot(pos, u_accretion_disc_normal);
    float zn = z / u_accretion_disc_height;
    float r = sqrt(max(dot(pos, pos) - z * z, 0.0));

    float r_in = 3.0 * u_black_hole_r_s;
    float r_out = r_in + u_accretion_disc_thickness;
    if (r < r_in || r > r_out) return 0.0;

    float verticalDensity = exp(- zn * zn / 2.0);
    if (verticalDensity < 0.01) return 0.0;

    float falloffThickness = u_accretion_disc_thickness * u_accretion_dist_edge_thickness_falloff_percent;
    float innerEdgeFade = smoothstep(r_in, r_in + falloffThickness, r);
    float outerEdgeFade = 1.0 - smoothstep(r_out - falloffThickness, r_out, r);
    float radial = pow(r / r_in, -1.5);

    float cloud = sampleCloudDensity(pos, r, z, zn, lod);
    return innerEdgeFade * outerEdgeFade * radial * verticalDensity * cloud;
}

vec2 rayAccretionDiscBoundingSphere(vec3 rayOrigin, vec3 rayDir) {
    float r_in = 3.0 * u_black_hole_r_s;
    float r = r_in + u_accretion_disc_thickness + u_black_hole_r_s;

    float b = dot(rayOrigin, rayDir);
    float h = b * b - (dot(rayOrigin, rayOrigin) - r * r);
    if (h < 0.0) return vec2(1.0, -1.0);
    h = sqrt(h);
    return vec2(-b - h, -b + h);
}

vec3 accretionDiscColour(vec3 pos, vec3 unitDir) {
    float r = length(pos);
    float r_in = 3.0 * u_black_hole_r_s;
    if (r < r_in) return vec3(0.0);

    // Shift factor
    float orbitalSpeed2 = u_black_hole_r_s / (2.0 * (r - u_black_hole_r_s));
    float num = sqrt((1.0 - u_black_hole_r_s / r) * (1.0 - orbitalSpeed2));
    vec3 unitVel = normalize(cross(u_accretion_disc_normal, pos));
    float denom = 1.0 - sqrt(orbitalSpeed2) * dot(unitVel, -unitDir);
    float shiftFactor = num / denom;

    // Compute temperature and apply shift
    float r_in_r = r_in / r;
    float tempProfile = pow(r_in_r, 0.75) * pow(max(0.0, 1.0 - sqrt(r_in_r)), 0.25);
    float tempEmit = u_accretion_disc_max_temperature * 2.05 * tempProfile;
    float temp = shiftFactor * tempEmit;

    // Apply intensity
    float intensity = pow(shiftFactor, 4.0);
    vec3 col = intensity * tannerHellandLinear(temp);

    return col;
}

// ----- Ray Marching -----

vec3 rayAcceleration(vec3 rayOrigin, float h2) {
    float r2 = dot(rayOrigin, rayOrigin);
    return -1.5 * u_black_hole_r_s * h2 * rayOrigin / (r2 * r2 * sqrt(r2));
}

void rk4(inout vec3 rayOrigin, inout vec3 rayDir, float h2, float stepSize) {
    vec3 k1p = rayDir;
    vec3 k1v = rayAcceleration(rayOrigin, h2);

    vec3 k2p = rayDir + 0.5 * stepSize * k1v;
    vec3 k2v = rayAcceleration(rayOrigin + 0.5 * stepSize * k1p, h2);

    vec3 k3p = rayDir + 0.5 * stepSize * k2v;
    vec3 k3v = rayAcceleration(rayOrigin + 0.5 * stepSize * k2p, h2);

    vec3 k4p = rayDir + stepSize * k3v;
    vec3 k4v = rayAcceleration(rayOrigin + stepSize * k3p, h2);

    rayOrigin += (stepSize / 6.0) * (k1p + 2.0 * k2p + 2.0 * k3p + k4p);
    rayDir += (stepSize / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
}

vec3 castRay(vec3 rayOrigin, vec3 unitRayDir) {
    rayOrigin -= u_black_hole_pos;
    vec3 bgColour = vec3(0.0);
    vec3 rayDir = u_light_speed * unitRayDir;

    // Constants for ray marching
    vec3 p = cross(rayOrigin, rayDir);
    float h2 = dot(p, p);
    float K = 1.5 * u_black_hole_r_s * h2;
    float cK = (K > 0.0) ? u_light_speed / K : MAX_FLOAT;

    // Check accretion disc bounding sphere
    vec2 t = rayAccretionDiscBoundingSphere(rayOrigin, unitRayDir);
    if (t.x > t.y) return bgColour;
    if (t.x > 0.0) rayOrigin += unitRayDir * 0.9 * t.x;

    float transmittance = 1.0;
    vec3 light = vec3(0.0);

    for (int it = 0; it < u_ray_marching_max_steps; it++) {
        float r2 = dot(rayOrigin, rayOrigin);
        float r = sqrt(r2);

        // Check event horizon or escape
        float r_exit = 3.0 * u_black_hole_r_s + u_accretion_disc_thickness + 1.0;
        if (r <= u_black_hole_r_s || (r > r_exit && dot(rayOrigin, rayDir) > 0.0)) break;

        float stepSize = clamp(u_ray_marching_step_size_factor * min(r / length(rayDir), cK * r2 * r2), u_ray_marching_min_step_size, u_ray_marching_max_step_size);

        // Update light energy and transmittance
        float density = sampleAccretionDiscDensity(rayOrigin, u_level_of_detail);
        if (density >= 0.001) {
            unitRayDir = normalize(rayDir);
            vec3 lightTransmittance = accretionDiscColour(rayOrigin, unitRayDir);
            float actualStepDist = stepSize * length(rayDir);
            float stepTransmittance = exp(-density * u_accretion_dist_extinction_coeff * actualStepDist);
            light += transmittance * (1.0 - stepTransmittance) * lightTransmittance;
            transmittance *= stepTransmittance;
        }

        if (transmittance < 0.01) break;

        // Update ray
        if (u_rk4) {
            rk4(rayOrigin, rayDir, h2, stepSize);
        } else {
            vec3 rayAcc = (-K / (r2 * r2 * r)) * rayOrigin;
            rayDir += stepSize * rayAcc;
            rayOrigin += stepSize * rayDir;
        }
    }

    return bgColour * transmittance + light;
}

void main() {
    // Compute ray position and direction vectors
    ivec2 windowPixel = ivec2(gl_FragCoord.xy);
    windowPixel.y = int(u_resolution.y - windowPixel.y);
    vec3 pixel = u_viewport_top_left + u_viewport_unit_x * windowPixel.x + u_viewport_unit_y * windowPixel.y;
    vec3 rayPos = u_camera_pos;
    vec3 unitRayDir = normalize(pixel - rayPos);

    // Cast ray to compute colour
    vec3 hdrColour = castRay(rayPos, unitRayDir);

    // Apply exposure, tone map, and gamma correction to final colour
    hdrColour *= u_exposure;
    vec3 mapped = acesToneMap(hdrColour);
    vec3 gammaCorrected = pow(mapped, vec3(1.0 / 2.2));

    // outputColor = vec4(hdrColour, 1.0);
    outputColor = vec4(gammaCorrected, 1.0);
}
