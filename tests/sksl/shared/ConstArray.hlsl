static float4 sk_FragColor;

struct SPIRV_Cross_Output
{
    float4 sk_FragColor : SV_Target0;
};

float4 main(float2 _18)
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}

void frag_main()
{
    float2 _14 = 0.0f.xx;
    sk_FragColor = main(_14);
}

SPIRV_Cross_Output main()
{
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.sk_FragColor = sk_FragColor;
    return stage_output;
}