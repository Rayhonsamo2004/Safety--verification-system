from ultralytics import YOLO

# Load trained model
model = YOLO("best.pt")

# Run evaluation with the correct dataset path
metrics = model.val(data="D:/final/data.yaml")  # Update path correctly
