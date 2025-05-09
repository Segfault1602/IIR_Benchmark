clear all;clf;

N = 23;
[z,p,k] = butter(N, 0.25);
sos = zp2sos(z,p,k, 'up','inf');

[x, fs]= audioread("../x1.wav");

y = sosfilt(sos, x);

freqz(sos);

audiowrite("../x1_filtered.wav", y, fs, 'BitsPerSample',32);

% Write the sos coefficients in a way that can be read by the C++ code
fid = fopen("sos_coeffs.h", "w");
fprintf(fid, "#pragma once\n");
fprintf(fid, "\n");
fprintf(fid, "#include <array>\n\n");
fprintf(fid, "// This file is generated by the script generate_test.m\n");
fprintf(fid, "// The coefficients are in the format: {b0, b1, b2, a0, a1, a2}\n");
fprintf(fid, "constexpr std::array<std::array<float, 6>, %d> sos_coeffs = {{\n", size(sos, 1));
for i = 1:size(sos, 1)
    fprintf(fid, "    {%.8f, %.8f, %.8f, %.8f, %.8f, %.8f},\n", sos(i, :));
end
fprintf(fid, "}};\n");

fclose(fid);