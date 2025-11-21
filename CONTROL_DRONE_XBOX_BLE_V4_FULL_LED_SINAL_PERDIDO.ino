#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include <Adafruit_NeoPixel.h>

// --- CONFIGURAÇÕES ---
//- para encontrear o MAC address do seu joystick instalar app android BLE Scan e colocar o dispositivo em modo de pareamento
#define XBOX_MAC_ADDR "MAC address do seu dispositivo".
#define NEOPIXEL_PIN 8  
#define NUM_PIXELS   1 

// Configurações de Voo
#define THROTTLE_MIN 0
#define THROTTLE_MAX 255

XboxSeriesXControllerESP32_asukiaaa::Core xboxController(XBOX_MAC_ADDR);
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Variáveis de Estado do Drone
int aceleracaoAtual = 0; // Valor que vai para os motores (0-255)
bool emFailsafe = false; // Indica se está em modo de emergência

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.clear();
  pixels.show();
  
  Serial.println("Drone Iniciado. Aguardando conexão...");
  xboxController.begin(); 
}

void loop() {
  xboxController.onLoop();

  // ============================================================
  // CENÁRIO 1: CONEXÃO ESTABELECIDA (PILOTO NO CONTROLE)
  // ============================================================
  if (xboxController.isConnected()) {
    
    // Se estavamos em failsafe antes, avisa que voltou!
    if (emFailsafe) {
       Serial.println("SINAL RECUPERADO! Retomando controle.");
       // Vibra o controle para avisar o piloto (Força motor pequeno, Força motor grande, Duração ms)
       xboxController.xboxNotif.vibrate(100, 100, 500); 
       emFailsafe = false;
    }

    // --- Leitura do Acelerador (Stick Esquerdo) ---
    // Mapeia o analógico (0-65535) para o motor (0-255)
    int comandoPiloto = map(xboxController.xboxNotif.joyLVert, 0, 65535, THROTTLE_MIN, THROTTLE_MAX);
    
    // Zona morta simples para garantir zero
    if (comandoPiloto < 10) comandoPiloto = 0;

    // Atualiza a aceleração do drone imediatamente
    aceleracaoAtual = comandoPiloto;

    // Feedback Visual:
    // Verde = Conectado e Seguro
    // Brilho = Potência do Motor
    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); 
    pixels.setBrightness(aceleracaoAtual); 
    pixels.show();

    Serial.print("Modo: MANUAL | Motor: ");
    Serial.println(aceleracaoAtual);
  
  } 
  // ============================================================
  // CENÁRIO 2: PERDA DE SINAL (MODO AUTÔNOMO / FAILSAFE)
  // ============================================================
  else {
    emFailsafe = true;
    
    // A Lógica de Pouso Suave:
    // Não corta o motor de vez! Reduz gradualmente.
    
    if (aceleracaoAtual > 0) {
      aceleracaoAtual = aceleracaoAtual - 1; // Reduz a velocidade lentamente
      
      // Feedback Visual: Roxo (Cor de Emergência)
      pixels.setPixelColor(0, pixels.Color(255, 0, 255)); 
      pixels.setBrightness(aceleracaoAtual); // O brilho vai caindo sozinho
      pixels.show();
      
      Serial.print("!!! ALERTA: SINAL PERDIDO !!! Pousando... Motor: ");
      Serial.println(aceleracaoAtual);
      
      // Delay controla a velocidade da descida (quanto maior, mais suave o pouso)
      delay(50); 
    } 
    else {
      // Motor chegou a zero. Drone no chão (ou caiu).
      aceleracaoAtual = 0;
      
      // Pisca Vermelho indicando "Perdido e Parado"
      pixels.setBrightness(100);
      if ((millis() / 200) % 2 == 0) {
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      } else {
        pixels.clear();
      }
      pixels.show();
      Serial.println("SINAL PERDIDO. Drone parado.");
    }
  }
}
