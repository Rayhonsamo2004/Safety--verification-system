import cv2
import torch
import gradio as gr
from ultralytics import YOLO
import numpy as np

# Load both trained YOLO models
model = YOLO("best.pt")  

# Define class labels for safety
SAFE_CLASSES = ["Helmet", "Vest", "Hardhat", "Safety Vest", "Mask"]  
DETECTED_CLASSES = ["Person", "Hardhat", "Safety Vest", "Mask", "No-Hardhat", "No-Safety Vest", "No-Mask"]

def detect_safety(image):
    image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    # Run detection using both models
    results = model(image_rgb)
    detected_objects = set()
    violations = []

    # Process results from fire & safety model
    for result in results:
        for box in result.boxes:
            x1, y1, x2, y2 = map(int, box.xyxy[0])
            class_id = int(box.cls[0])
            confidence = float(box.conf[0])
            label = result.names[class_id]
            detected_objects.add(label)

            # Assign color based on safety compliance
            color = (0, 255, 0) if label in SAFE_CLASSES else (0, 0, 255)
            cv2.rectangle(image, (x1, y1), (x2, y2), color, 2)
            cv2.putText(image, f"{label}: {confidence:.2f}", (x1, y1 - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
                        
    # Safety violation checks
    if "Fire" in detected_objects:
        cv2.putText(image, "⚠️ Fire Detected!", (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
    if "Smoke" in detected_objects:
        cv2.putText(image, "⚠️ Smoke Detected!", (50, 80), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
    
    if "Person" in detected_objects:
        missing_equipment = []
        if "Hardhat" not in detected_objects and "No-Hardhat" in detected_objects:
            missing_equipment.append("helmet")
        if "Safety Vest" not in detected_objects and "No-Safety Vest" in detected_objects:
            missing_equipment.append("safety vest")
        if "Mask" not in detected_objects and "No-Mask" in detected_objects:
            missing_equipment.append("mask")
        
        if missing_equipment:
            violation_message = f"⚠️ WARNING: Person detected without {' and '.join(missing_equipment)}!"
            violations.append(violation_message)
            cv2.putText(image, violation_message, (50, 110), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

    # Resize image for display
    resized_image = cv2.resize(image, (800, 600))

    return resized_image

# Gradio UI
iface = gr.Interface(
    fn=detect_safety,
    inputs=gr.Image(type="numpy"),
    outputs=gr.Image(type="numpy"),
    title="Safety & Fire Detection System",
    description="Upload an image to detect fire, smoke, and safety gear compliance (helmet, vest, mask)."
)

if __name__ == "__main__":
    iface.launch(share=True)
