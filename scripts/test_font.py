import cv2
import numpy as np

font_path = "assets/font3x5.png"
font_size = (3, 5)
text = "Hello, World!"

font_image = cv2.imread(font_path, cv2.IMREAD_COLOR_RGB)
if font_image is None:
    raise FileNotFoundError(f"Font image not found at path: {font_path}")

blank_image = 0 * np.ones((50, 300, 3), dtype=np.uint8)

for i, char in enumerate(text):
    ascii_index = ord(char)
    cell_size = (font_size[0] + 2, font_size[1] + 2)
    cols = font_image.shape[1] // cell_size[0]
    row = ascii_index // cols
    col = ascii_index % cols
    x = col * cell_size[0]
    y = row * cell_size[1]
    char_image = font_image[y:y + cell_size[1], x:x + cell_size[0]]
    pos_x = i * (font_size[0] + 2)
    pos_y = 0
    blank_image[pos_y:pos_y + cell_size[1], pos_x:pos_x + cell_size[0]] = char_image
blank_image = blank_image[0:cell_size[1], 0:len(text)*cell_size[0]]
scale = 4
blank_image = cv2.resize(blank_image, (blank_image.shape[1]*scale, blank_image.shape[0]*scale), interpolation=cv2.INTER_NEAREST)
cv2.imshow("Rendered Text", blank_image)
cv2.waitKey(0)
cv2.destroyAllWindows()
    
