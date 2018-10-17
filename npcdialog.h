const uint8_t sampleDialog[] = {
0x01, 0x44, 0x6f, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x77, 0x61, 
0x6e, 0x74, 0x20, 0x74, 0x6f, 0x20, 0x62, 0x61, 0x74, 0x74, 
0x6c, 0x65, 0x3f, 0x00, 0x02, 0x03, 0x01, 0x1e, 0x00, 0x05, 0x00
};
const uint8_t doctorDialog[] = {
0x01, 0x41, 0x72, 0x65, 0x20, 0x79, 0x6f, 0x75, 0x72, 0x20, 
0x6d, 0x6f, 0x6e, 0x73, 0x74, 0x65, 0x72, 0x73, 0x20, 0x68, 
0x75, 0x72, 0x74, 0x3f, 0x00, 0x02, 0x03, 0x01, 0x59, 0x00, 
0x01, 0x57, 0x65, 0x20, 0x77, 0x69, 0x6c, 0x6c, 0x20, 0x74, 
0x61, 0x6b, 0x65, 0x20, 0x63, 0x61, 0x72, 0x65, 0x20, 0x6f, 
0x66, 0x20, 0x74, 0x68, 0x65, 0x6d, 0x2e, 0x00, 0x06, 0x01, 
0x59, 0x6f, 0x75, 0x72, 0x20, 0x6d, 0x6f, 0x6e, 0x73, 0x74, 
0x65, 0x72, 0x73, 0x20, 0x61, 0x72, 0x65, 0x20, 0x68, 0x65, 
0x61, 0x6c, 0x65, 0x64, 0x2e, 0x00, 0x04, 0x77, 0x00, 0x01, 
0x54, 0x68, 0x61, 0x74, 0x20, 0x69, 0x73, 0x20, 0x61, 0x6c, 
0x77, 0x61, 0x79, 0x73, 0x20, 0x67, 0x6f, 0x6f, 0x64, 0x20, 
0x74, 0x6f, 0x20, 0x68, 0x65, 0x61, 0x72, 0x2e, 0x00, 0x00
};
const uint8_t worriedDialog[] = {
0x01, 0x49, 0x20, 0x77, 0x61, 0x6e, 0x74, 0x20, 0x74, 0x6f, 
0x20, 0x62, 0x61, 0x74, 0x74, 0x6c, 0x65, 0x20, 0x62, 0x75, 
0x74, 0x2e, 0x2e, 0x2e, 0x00, 0x01, 0x6d, 0x79, 0x20, 0x63, 
0x61, 0x74, 0x65, 0x79, 0x65, 0x20, 0x69, 0x73, 0x20, 0x76, 
0x65, 0x72, 0x79, 0x20, 0x73, 0x69, 0x63, 0x6b, 0x2e, 0x00, 0x00
};
const uint8_t introDialog[] = {
0x01, 0x57, 0x65, 0x6c, 0x63, 0x6f, 0x6d, 0x65, 0x2e, 0x20, 
0x59, 0x6f, 0x75, 0x72, 0x20, 0x61, 0x63, 0x63, 0x69, 0x64, 
0x65, 0x6e, 0x74, 0x20, 0x77, 0x61, 0x73, 0x00, 0x01, 0x75, 
0x6e, 0x66, 0x6f, 0x72, 0x74, 0x75, 0x6e, 0x61, 0x74, 0x65, 
0x2e, 0x20, 0x57, 0x65, 0x20, 0x77, 0x69, 0x6c, 0x6c, 0x20, 
0x74, 0x65, 0x61, 0x63, 0x68, 0x00, 0x01, 0x79, 0x6f, 0x75, 
0x20, 0x74, 0x6f, 0x20, 0x63, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 
0x6c, 0x20, 0x79, 0x6f, 0x75, 0x72, 0x20, 0x6d, 0x6f, 0x6e, 
0x73, 0x74, 0x65, 0x72, 0x2e, 0x00, 0x01, 0x57, 0x68, 0x61, 
0x74, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x69, 0x73, 
0x20, 0x79, 0x6f, 0x75, 0x72, 0x20, 0x6d, 0x6f, 0x6e, 0x73, 
0x74, 0x65, 0x72, 0x3f, 0x00, 0x07, 0x01, 0x48, 0x61, 0x76, 
0x65, 0x20, 0x61, 0x20, 0x6c, 0x6f, 0x6f, 0x6b, 0x20, 0x61, 
0x72, 0x6f, 0x75, 0x6e, 0x64, 0x2e, 0x00, 0x00
};
const uint8_t mentorDialog[] = {
0x01, 0x49, 0x20, 0x63, 0x72, 0x65, 0x61, 0x74, 0x65, 0x64, 
0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x74, 0x6f, 0x77, 0x6e, 
0x20, 0x74, 0x6f, 0x20, 0x74, 0x72, 0x61, 0x69, 0x6e, 0x00, 
0x01, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x6d, 0x61, 0x73, 
0x74, 0x65, 0x72, 0x73, 0x2e, 0x20, 0x46, 0x69, 0x72, 0x73, 
0x74, 0x20, 0x74, 0x72, 0x79, 0x20, 0x74, 0x6f, 0x00, 0x01, 
0x6c, 0x65, 0x61, 0x72, 0x6e, 0x20, 0x61, 0x62, 0x6f, 0x75, 
0x74, 0x20, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x73, 0x74, 
0x72, 0x69, 0x6b, 0x65, 0x73, 0x2e, 0x00, 0x00
};
