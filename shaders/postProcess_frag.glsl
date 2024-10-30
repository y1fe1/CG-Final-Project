#version 410
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene; // 场景纹理

//void main()
//{
//    // 简单的模糊效果，通过采样周围像素并取平均值
//    vec3 color = vec3(0.0);
//    float offset = 1.0 / 300.0; // 偏移量用于控制模糊强度
//
//    for (int x = -1; x <= 1; x++) {
//        for (int y = -1; y <= 1; y++) {
//            color += texture(scene, TexCoords + vec2(x, y) * offset).rgb;
//        }
//    }
//    color /= 9.0; // 取平均值
//    FragColor = vec4(color, 1.0);
//}
 //postProcess_frag.glsl
//#version 410
//out vec4 FragColor;
//in vec2 TexCoords;
//uniform sampler2D scene;

//void main() {
//    FragColor = texture(scene, TexCoords);
//}


// 在 postProcess_frag.glsl 中测试
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // 输出纯红色，确保四边形渲染正确
}
