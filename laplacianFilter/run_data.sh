gcc -o edge_detector edge_detector.c

find ./photos-1 -maxdepth 1 -type f -name "*.ppm" -exec ./edge_detector {} +