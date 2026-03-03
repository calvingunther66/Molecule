import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
import os

# Neural Network scaled up to fit the ~264KB flash limit of Pico
class RetrosynthPolicyNet(nn.Module):
    def __init__(self, input_size=1024, hidden_size=256, output_size=50):
        super(RetrosynthPolicyNet, self).__init__()
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        x = self.fc1(x)
        x = self.relu(x)
        x = self.fc2(x)
        return x

def train_model(data_dir="../data", epochs=20, batch_size=512):
    X_train = np.load(os.path.join(data_dir, "X_train.npy"))
    y_train = np.load(os.path.join(data_dir, "y_train.npy"))

    X_tensor = torch.tensor(X_train, dtype=torch.float32)
    y_tensor = torch.tensor(y_train, dtype=torch.long)

    dataset = torch.utils.data.TensorDataset(X_tensor, y_tensor)
    dataloader = torch.utils.data.DataLoader(dataset, batch_size=batch_size, shuffle=True)

    model = RetrosynthPolicyNet()
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=0.005)

    print(f"Starting training on {len(X_train)} samples...")

    for epoch in range(epochs):
        epoch_loss = 0.0
        correct = 0
        total = 0
        for batch_X, batch_y in dataloader:
            optimizer.zero_grad()
            outputs = model(batch_X)
            loss = criterion(outputs, batch_y)
            loss.backward()
            optimizer.step()

            epoch_loss += loss.item()
            _, predicted = torch.max(outputs.data, 1)
            total += batch_y.size(0)
            correct += (predicted == batch_y).sum().item()

        if (epoch + 1) % 10 == 0:
            print(f"Epoch {epoch+1}/{epochs} | Loss: {epoch_loss/len(dataloader):.4f} | Acc: {100 * correct / total:.2f}%")

    os.makedirs("models", exist_ok=True)
    torch.save(model.state_dict(), "models/policy_net.pth")
    print("Model saved to models/policy_net.pth")

if __name__ == "__main__":
    train_model()
