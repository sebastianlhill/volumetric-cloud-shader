#version 330 core
in vec2 vUV;
out vec4 fragColor;

uniform vec2 u_resolution;
uniform float u_time;
uniform sampler3D uCloudShape; 
uniform sampler3D uCloudErosion;

#define CLOUD_MAX 400.0
#define CLOUD_MIN 200.0 

#define MAX_STEPS 128




float remap(float x, float a, float b, float c, float d) {
    return c + (x - a) / (b - a) * (d - c);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// query when a ray intersects a box, returning near point and far point 
// we can then march over near and far, which saves alot of wasted marching 
vec2 rayBox(vec3 ro, vec3 rd, float boxMin, float boxMax) {
  // the box min and box max can be thought of as xz planes at some y value 
  // so we are looking for the intersection of the ray at those planes 
  
  // if the ray is parallel to planes, cant divide 
  if (abs(rd.y) < 1e-6) {
    if (ro.y >= boxMin && ro.y <= boxMax){
      // origin is between planes 
      return vec2(0.0, 1e20);
    }
    return vec2(-1.0, -1.0);
  }
  // ray formula 
  // p(t) = origin + (t * direction)
  float tBottom = (boxMin - ro.y) / rd.y;
  float tTop    = (boxMax - ro.y) / rd.y;

  // whether we are above cloud layer or below changes which is 
  // near and which is far 
  float tNear = min(tBottom, tTop);
  float tFar  = max(tBottom, tTop);
      
  if (tFar <= 0.0) return vec2(-1.0, -1.0);

  // Clamp near to 0 so we don't report hits behind the camera.
  tNear = max(tNear, 0.0); 
  
  // if plane is behind us, we dont use it  
  return vec2(tNear, tFar);
}

float heightGradient(float y) {
  float h = (y - CLOUD_MIN) / (CLOUD_MAX - CLOUD_MIN);
  float bottomFade = smoothstep(0.0, 0.15, h);
  float topFade    = smoothstep(1.0, 0.70, h);

  return bottomFade * topFade;
}

float cloudDensity(vec3 p){
  vec4 shape = texture(uCloudShape, p * 0.0015);
  float baseCloud  = shape.r;
  float lowFreqFBM = shape.g * 0.625 + shape.b * 0.25 + shape.a * 0.125;

  baseCloud = remap(baseCloud, -(1.0 - lowFreqFBM), 1.0, 0.0, 1.0);

  baseCloud *= heightGradient(p.y);

  vec4 erosion = texture(uCloudErosion, p *0.01);
  float highFreqFBM = erosion.r * 0.625 + erosion.g * 0.25 + erosion.b * 0.125;

  float erosionStrength = mix(1.0 - highFreqFBM, 0.0, baseCloud);
  float finalDensity = remap(baseCloud, erosionStrength, 1.0, 0.0, 1.0);

  return clamp(finalDensity, 0.0, 1.0) * 0.012;

}

float inScatter(vec3 p){
  return 0.9;
}

vec3 skyColor(vec3 rd) {
  float h = clamp(rd.y * 0.5 + 0.5, 0.0, 1.0);
  vec3 horizon = vec3(0.65, 0.80, 1.00);
  vec3 zenith = vec3(0.15, 0.35, 0.75);
  return mix(horizon, zenith, smoothstep(0.0, 1.0, h));
}
void main() {
    vec2 uv = 2.0 * vUV - 1.0;
    uv.x *= u_resolution.x / u_resolution.y;
    float time = u_time * 0.6;  
   
    vec3 camPos = vec3(20.0,18.0,-50.0);
    vec3 camX   = vec3( 1.0, 0.0, 0.0);
    vec3 camY   = vec3( 0.0, 1.0, 0.0);
    vec3 camZ   = vec3( 0.0, 0.0, 1.0);


    // ray origin 
    vec3 ro = camPos;
    // ray direction, forward is pos z 
    vec3 rd = normalize(uv.x*camX + uv.y*camY + camZ);
    
    vec3 albedo = vec3( 0.0, 0.0, 0.0);
    vec3 background = skyColor(rd);
    vec3 color      = background;
    

    vec2 hit = rayBox(ro, rd, CLOUD_MIN, CLOUD_MAX);
    float tNear = hit.x;
    float tFar  = hit.y;
    
    float g = 0.0;
    // we start at 1.0, 100%. as we go through cloud, our transmittance decays
    float transmittance = 1.0;
    float stepSize = (tFar - tNear) / MAX_STEPS;

    float t = 0.0;
    vec3 x = vec3(0.0,0.0,0.0);
    float sigma_t = 0;
    float stepTrans = 0;
    float luminance = 0;

    if (tFar > 0.0) {
      for (int i = 0; i < MAX_STEPS; ++i){
        t = tNear + (i + 0.5) * stepSize; 
        x = ro + t * rd;
        sigma_t = cloudDensity(x);
        // transmittance over this segment 
        stepTrans = exp(-sigma_t * stepSize);
        // accumulate in scattered light 
        luminance += transmittance * (1.0 - stepTrans) * inScatter(x);
       
        transmittance *= stepTrans; 
        if (transmittance < 0.01) break; 
      }

      color = luminance + transmittance * background;
    }

    // Gamma correction
    color = pow(color, vec3(1.0/2.2)); // simple linear to gamma, exposure of 1.0

    vec4 s = texture(uCloudShape, vec3(uv * 4.0, 0.5));
    //fragColor = vec4(vec3(uv*4.0,0.5), 1.0);   // → Perlin-Worley only
    // fragColor = vec4(vec3(s.g), 1.0);   // → Worley FBM 0
    // fragColor = vec4(vec3(s.b), 1.0);   // → Worley FBM 1
    // fragColor = vec4(vec3(s.a), 1.0);   // → Worley FBM 2
    fragColor = vec4(color, 1.0);
    //fragColor = vec4(vec3(s.g), 1.0);   // → Worley FBM 0
    //fragColor = vec4(vec3(s.r), 1.0);   // → Perlin-Worley only
}
