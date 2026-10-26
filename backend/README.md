# Backend: Supabase + Razorpay

The primary prototype flow uses Razorpay **Payment Links** and three Supabase Edge Functions:

- `create-payment-link`: creates a pending transaction and a Razorpay Payment Link.
- `razorpay-webhook`: verifies the Razorpay webhook signature and marks a matching pending transaction as paid.
- `transaction-status`: returns transaction status to the ESP32; its PATCH path marks a paid transaction as dispensed.
- `create-order`: older alternative using the Razorpay Orders API. It points to a `checkout` function that is not included here, so it is not the documented active flow.

The TypeScript source files are snapshots reconstructed from deployed Edge Function source. Review and test before redeployment.

## Environment variables

Set these as Supabase Edge Function secrets. Never commit secret values or put them in ESP32 firmware.

| Variable | Purpose |
|---|---|
| `SUPABASE_URL` | Supabase project URL |
| `SUPABASE_SERVICE_ROLE_KEY` | Server-side database access; keep private |
| `SUPABASE_SECRET_KEYS` | Optional fallback supported by the current source; if used, the value must be JSON with a `default` property |
| `RAZORPAY_KEY_ID` | Razorpay API key ID |
| `RAZORPAY_KEY_SECRET` | Razorpay API secret |
| `RAZORPAY_WEBHOOK_SECRET` | Secret configured for the webhook endpoint |

See [`.env.example`](.env.example) for names and placeholders only.

## Deploy active functions

After linking the correct Supabase project and setting its secrets:

```bash
supabase functions deploy create-payment-link
supabase functions deploy razorpay-webhook
supabase functions deploy transaction-status
```

The legacy `create-order` function is not required for the Payment Links flow. Do not use it as the primary flow unless you also implement and secure the missing `checkout` endpoint and update the client.

Set secrets through the Supabase CLI, for example:

```bash
supabase secrets set SUPABASE_URL="https://YOUR_PROJECT_REF.supabase.co"
supabase secrets set SUPABASE_SERVICE_ROLE_KEY="YOUR_SERVER_SIDE_KEY"
supabase secrets set RAZORPAY_KEY_ID="YOUR_RAZORPAY_TEST_KEY_ID"
supabase secrets set RAZORPAY_KEY_SECRET="YOUR_RAZORPAY_TEST_KEY_SECRET"
supabase secrets set RAZORPAY_WEBHOOK_SECRET="YOUR_WEBHOOK_SECRET"
```

Replace placeholders only in your local terminal or trusted secret manager. Do not paste actual secrets into repository files or screenshots.

## Razorpay webhook

1. Enable **Test Mode** in Razorpay.
2. Set the deployed `razorpay-webhook` function URL as the webhook endpoint.
3. Configure the same webhook secret as `RAZORPAY_WEBHOOK_SECRET`.
4. Subscribe to `payment_link.paid`.
5. Make a test payment and verify the database row changes from `pending` to `paid`.

The function validates an HMAC-SHA256 signature over the raw request body before processing the event.

## API behaviour

### Create a payment link

`POST /functions/v1/create-payment-link`

```json
{ "candy_id": 1 }
```

Supported IDs: `1 → ₹5`, `2 → ₹10`, `3 → ₹20`. The successful response contains `transaction_id`, `payment_url`, `amount`, and `candy_id`.

### Read transaction status

`GET /functions/v1/transaction-status?id=TRANSACTION_UUID`

Returns `id`, `status`, `candy_id`, and `amount_inr`.

### Mark as dispensed

`PATCH /functions/v1/transaction-status?id=TRANSACTION_UUID`

The current code updates a row only if its status is `paid`, setting `status=dispensed` and `dispensed_at`. **This endpoint currently has no authentication. Do not expose this design publicly or use it for real payments.**

## Security limitations

This is educational prototype code, not a production payment implementation:

- JWT verification is disabled on the deployed functions.
- CORS allows all origins.
- Transaction lookup is based on a supplied ID, and the PATCH transition is unauthenticated.
- The service-role key bypasses normal row-level security and must remain server-side.
- The webhook does not store processed event IDs for replay protection/idempotency.
- A client must not be allowed to authorize dispensing arbitrary transactions.

Before public or real-money use, add authenticated device requests, transaction/device authorization, strict state-machine validation, rate limiting, webhook replay protection, audit logs, and automated tests. Use Razorpay test mode only for this prototype.
