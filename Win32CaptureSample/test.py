import cv2
import pytesseract
import os
import numpy as np
os.environ["PATH"] += r"D:\vcpkg\installed\x64-windows\tools\tesseract"
# Load the image
img = cv2.imread('D:/D3Hunt/test_hint7.png')
image = cv2.imread('D:/D3Hunt/test_pos_withcolor.png')
# gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
# _, thresh = cv2.threshold(gray, 127, 255, cv2.THRESH_BINARY)
# cv2.imshow("", thresh)
# cv2.waitKey(0)
# from pytesseract import Output
# detected = []
# rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
# results = pytesseract.image_to_string(rgb, lang='fra+eng', config="--psm 11 --oem 3")
# results = results.replace("\n\n", " ")
# print(results.strip())
# for i in range(0, len(results["text"])):
# 	# extract the bounding box coordinates of the text region from
# 	# the current result
# 	x = results["left"][i]
# 	y = results["top"][i]
# 	w = results["width"][i]
# 	h = results["height"][i]
# 	# extract the OCR text itself along with the confidence of the
# 	# text localization
# 	text = results["text"][i]
# 	conf = int(float(results["conf"][i]))
# 	if conf > 10:
# 		# display the confidence and text to our terminal
# 		print("Confidence: {}".format(conf))
# 		detected.append(text)
# 		print("Text: {}".format(text))
# 		print("")
# print(" ".join(detected))
# items = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
# contours = items[0] if len(items) == 2 else items[1]

# img_contour = img.copy()

# detected = ""
# cs = []
# for c in contours:
#     print(c)
#     x, y, w, h = cv2.boundingRect(c)
    
#     area = cv2.contourArea(c)
#     if area < 1 or  area > 100:
#         continue
#     cs.append((x, y, w, h))
# cs = sorted(cs, key=lambda x: x[0])
# for x, y, w, h in cs:
#     base = np.ones(thresh.shape, dtype=np.uint8)

#     base[y:y+h, x:x+w] = thresh[y:y+h, x:x+w]
#     segment = cv2.bitwise_not(base)
#     if cv2.countNonZero(segment) < 2:
#         continue
#     custom_config = r'-l eng --oem 3 --psm 10 -c tessedit_char_whitelist="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\'" '
#     c = pytesseract.image_to_string(segment, config=custom_config)
#     detected = detected + c.replace('\n','')

# print("detected: " + detected)

# # Convert the image to grayscale
# gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

# # # Apply some preprocessing (optional)
# # gray = cv2.medianBlur(gray, 3)
# cv2.imshow("t", gray)
# cv2.waitKey(0)
# whiteList = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefgijklmnopqrstuvwxyz Ééèêçà'ô[]()?/,-"
# tess_config=f'-c tessedit_char_whitelist="{whiteList}" --psm 11'
# # Use Tesseract to extract text
# text = pytesseract.image_to_string(gray, config=tess_config)

# print("Extracted Text: ", text)

# import cv2
# import sys
# import numpy as np

def nothing(x):
    pass

# Load in image
# image = cv2.imread('D:/D3Hunt/test_hint2.png')
# gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
# ret, thresh = cv2.threshold(gray, 250, 255, cv2.THRESH_BINARY)
# cv2.imshow("", thresh)
# cv2.waitKey(0)
# non_zero = cv2.countNonZero(thresh)

# print(non_zero)
# Create a window
cv2.namedWindow('image', cv2.WINDOW_NORMAL)

# create trackbars for color change
cv2.createTrackbar('HMin','image',0,179,nothing) # Hue is from 0-179 for Opencv
cv2.createTrackbar('SMin','image',0,255,nothing)
cv2.createTrackbar('VMin','image',0,255,nothing)
cv2.createTrackbar('HMax','image',0,179,nothing)
cv2.createTrackbar('SMax','image',0,255,nothing)
cv2.createTrackbar('VMax','image',0,255,nothing)

# Set default value for MAX HSV trackbars.
cv2.setTrackbarPos('HMax', 'image', 179)
cv2.setTrackbarPos('SMax', 'image', 255)
cv2.setTrackbarPos('VMax', 'image', 255)

# Initialize to check if HSV min/max value changes
hMin = sMin = vMin = hMax = sMax = vMax = 0
phMin = psMin = pvMin = phMax = psMax = pvMax = 0

output = image
wait_time = 33

while(1):

    # get current positions of all trackbars
    hMin = cv2.getTrackbarPos('HMin','image')
    sMin = cv2.getTrackbarPos('SMin','image')
    vMin = cv2.getTrackbarPos('VMin','image')

    hMax = cv2.getTrackbarPos('HMax','image')
    sMax = cv2.getTrackbarPos('SMax','image')
    vMax = cv2.getTrackbarPos('VMax','image')

    # Set minimum and max HSV values to display
    lower = np.array([hMin, sMin, vMin])
    upper = np.array([hMax, sMax, vMax])

    # Create HSV Image and threshold into a range.
    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    mask = cv2.inRange(hsv, lower, upper)
    output = cv2.bitwise_and(image,image, mask= mask)

    # Print if there is a change in HSV value
    if( (phMin != hMin) | (psMin != sMin) | (pvMin != vMin) | (phMax != hMax) | (psMax != sMax) | (pvMax != vMax) ):
        print("(hMin = %d , sMin = %d, vMin = %d), (hMax = %d , sMax = %d, vMax = %d)" % (hMin , sMin , vMin, hMax, sMax , vMax))
        phMin = hMin
        psMin = sMin
        pvMin = vMin
        phMax = hMax
        psMax = sMax
        pvMax = vMax

    # Display output image
    cv2.imshow('image',output)

    # Wait longer to prevent freeze for videos.
    if cv2.waitKey(wait_time) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()