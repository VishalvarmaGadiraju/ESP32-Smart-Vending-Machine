# Database schema notes

The Edge Functions use `public.vending_transactions`. The deployed schema could not be independently exported while preparing these files, so the table below lists columns referenced by function source; it is not a verified migration.

| Column | Observed purpose |
|---|---|
| `id` | Transaction identifier returned to the client/ESP32 |
| `candy_id` | Selected item: 1, 2, or 3 |
| `amount_inr` | Price in rupees: 5, 10, or 20 |
| `status` | Observed values: `pending`, `paid`, `failed`, `dispensed` |
| `razorpay_order_id` | Stores the Razorpay Payment Link ID in the active flow |
| `razorpay_payment_id` | Payment ID recorded by the webhook |
| `paid_at` | Timestamp set when payment is confirmed |
| `dispensed_at` | Timestamp set when a paid transaction is marked dispensed |

Exact SQL types, defaults, constraints, indexes, and row-level security policies are not asserted here because the live schema was not verified. Export the actual schema from Supabase before creating or applying a migration.

## Recommended future constraints

- Primary key on `id`, using the identifier type expected by the firmware.
- Check constraint limiting `candy_id` to 1, 2, or 3.
- Check constraint limiting status to the intended state values.
- Non-negative amount constraint and indexes for transaction lookup and Razorpay identifiers.
- Server-side-only writes and appropriate row-level security policies.
- Idempotent state transitions so one transaction cannot dispense more than once.
