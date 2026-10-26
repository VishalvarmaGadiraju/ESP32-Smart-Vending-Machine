# ESP32 Smart Vending Machine

An ESP32-based smart vending machine prototype combining physical candy-selection buttons, a mobile-friendly web interface, Razorpay **test-mode** payments, Supabase transaction tracking, an OLED status display, and servo-controlled dispensing.

> **Status:** Student prototype tested with Razorpay test-mode payment flow. This is not a production-ready payment terminal; no real-money payment capability is claimed.

## Features

- Three candy options priced at ₹5, ₹10, and ₹20.
- ESP32-hosted web interface and physical selection buttons.
- Supabase Edge Functions create payment links and track transaction status.
- Razorpay Payment Links and signed webhook confirmation.
- ESP32 checks payment status before dispensing.
- 0.96-inch OLED display and three SG90 servo motors for the dispensing lanes.

## System flow

```text
Candy selection (button or web page)
                ↓
               ESP32
                ↓
Supabase creates a pending transaction
                ↓
Razorpay test payment link
                ↓
Signed Razorpay webhook confirms payment
                ↓
Supabase marks transaction paid
                ↓
ESP32 polls transaction status
                ↓
Servo dispenses the selected candy
```

## Hardware

| Component | Purpose |
|---|---|
| ESP32 DevKit | Main controller, Wi-Fi and backend requests |
| 0.96-inch OLED | Status and device information |
| 3 pushbuttons | Select the ₹5, ₹10 and ₹20 items |
| 3 SG90 servos | Demonstrate three dispensing lanes |
| Breadboard, jumpers and USB cable | Prototype wiring and programming |

Use a suitable external low-voltage supply for servos when needed; do not assume the ESP32 3.3 V pin can power multiple servos. Join grounds when using a separate servo supply. Keep the prototype isolated from mains electricity.

## Repository layout

```text
.
├── Vending_Machine_project.ino
├── README.md
├── LICENSE
└── backend/
    ├── README.md
    ├── .env.example
    ├── database-schema.md
    └── supabase/functions/
        ├── create-payment-link/index.ts
        ├── razorpay-webhook/index.ts
        ├── transaction-status/index.ts
        └── create-order/index.ts  # legacy alternative
```

## Setup overview

1. Install ESP32 board support and the libraries required by the firmware in Arduino IDE.
2. Configure Wi-Fi credentials locally in the firmware. **Never commit real passwords or API secrets.**
3. Configure Supabase Edge Function secrets as described in [`backend/README.md`](backend/README.md).
4. Create/verify the `vending_transactions` table using [`backend/database-schema.md`](backend/database-schema.md). The documented columns are based on function usage; exact SQL types and constraints still need to be checked against the live project.
5. Deploy the active functions: `create-payment-link`, `razorpay-webhook`, and `transaction-status`.
6. Configure a Razorpay **test-mode** webhook for the `payment_link.paid` event.
7. Upload firmware and test the full payment-to-dispense flow with test credentials.

## Security notes

- Never publish Razorpay secrets, Supabase service-role/secret keys, or Wi-Fi passwords. If a real credential was committed, rotate it; deleting it from the latest file does not erase it from Git history.
- The current prototype functions use public-access settings and broad CORS. The status endpoint accepts an unauthenticated PATCH request to mark a paid transaction as dispensed. This is **not suitable for public deployment or real payments** without authentication, authorization, replay protection, and stronger server-side validation.
- Keep all tests in Razorpay test mode until the security design is reviewed.

## Future improvements

Add authenticated device requests, transaction/device authorization, idempotent state transitions, rate limiting, webhook replay protection, structured logs, automated tests, servo calibration, jam detection, and secure configuration storage.

## License

MIT License — see [`LICENSE`](LICENSE).
