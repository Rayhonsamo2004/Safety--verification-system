import cv2
from ultralytics import YOLO

# Load the trained YOLO model
model = YOLO("best.pt")  # Replace with your trained model file

# Define class labels
SAFE_CLASSES = ["Hardhat", "Safety Vest", "Mask"]  # Safety equipment
DETECTED_CLASSES = ["Person", "Hardhat", "Safety Vest", "Mask", "No-Hardhat", "No-Safety Vest", "No-Mask"]

# Load the input image
image_path = "fire_image.jpg"  # Replace with your image path
image = cv2.imread(image_path)

# Perform object detection
results = model(image)

detected_objects = set()
violations = []

# Process detected objects
for result in results:
    for box in result.boxes:
        x1, y1, x2, y2 = map(int, box.xyxy[0])
        class_id = int(box.cls[0])
        confidence = float(box.conf[0])
        label = result.names[class_id]
        
        detected_objects.add(label)
        
        # Set bounding box color
        color = (0, 255, 0) if label in SAFE_CLASSES else (0, 0, 255)  # Green for safe, red for unsafe
        
        # Draw bounding box and label
        cv2.rectangle(image, (x1, y1), (x2, y2), color, 2)
        cv2.putText(image, f"{label}: {confidence:.2f}", (x1, y1 - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

# Safety violation checks
if "Person" in detected_objects:
    missing_equipment = []
    if "Hardhat" not in detected_objects and "No-Hardhat" in detected_objects:
        missing_equipment.append("helmet")
    if "Safety Vest" not in detected_objects and "No-Safety Vest" in detected_objects:
        missing_equipment.append("safety vest")
    if "Mask" not in detected_objects and "No-Mask" in detected_objects:
        missing_equipment.append("mask")
    
    if missing_equipment:
        violation_message = f"WARNING: Person detected without {' and '.join(missing_equipment)}!"
        print(violation_message)
        violations.append(violation_message)
        cv2.putText(image, violation_message, (50, 50),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

if "Fire" in detected_objects:
    print("⚠️ WARNING: Fire please evacuvate!")
    cv2.putText(image, "Fire Detected!", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

if "Smoke" in detected_objects:
    print("⚠️ WARNING: Smoke please evacuvate!")
    cv2.putText(image, "Smoke Detected!", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

# Resize image for display
resized_image = cv2.resize(image, (800, 600))

# Display results
cv2.imshow("Safety Detection", resized_image)
cv2.waitKey(0)
cv2.destroyAllWindows()
