#version 410
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene; // ��������

//void main()
//{
//    // �򵥵�ģ��Ч����ͨ��������Χ���ز�ȡƽ��ֵ
//    vec3 color = vec3(0.0);
//    float offset = 1.0 / 300.0; // ƫ�������ڿ���ģ��ǿ��
//
//    for (int x = -1; x <= 1; x++) {
//        for (int y = -1; y <= 1; y++) {
//            color += texture(scene, TexCoords + vec2(x, y) * offset).rgb;
//        }
//    }
//    color /= 9.0; // ȡƽ��ֵ
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


// �� postProcess_frag.glsl �в���
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // �������ɫ��ȷ���ı�����Ⱦ��ȷ
}
