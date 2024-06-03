Texture2D depthTexture;

float GetDepth(uint2 position)
{
  // out of bounds check
  position = clamp(position, uint2(0, 0), uint2(ViewportSize.xy) - uint2(1, 1));
  return depthTexture[position].r;
}

float GetDepth(float2 uv)
{
  // effects like screen space shadows, can get artefacts if a point sampler is used
  return depthTexture.SampleLevel(LinearClampSampler, uv, 0).r;
}

float GetLinearDepth(uint2 pos)
{
  return LinearizeZBufferDepth(GetDepth(pos));
}

float GetLinearDepth(float2 uv)
{
  return LinearizeZBufferDepth(GetDepth(uv));
}
