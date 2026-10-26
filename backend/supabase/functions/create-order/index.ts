// LEGACY / ALTERNATIVE FLOW.
// The active prototype uses create-payment-link + razorpay-webhook.
// This function points to a "checkout" Edge Function that is not included here.
import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const CANDY_PRICES: Record<number, number> = { 1: 5, 2: 10, 3: 20 };

Deno.serve(async (req: Request) => {
  if (req.method !== "POST") {
    return new Response(JSON.stringify({ error: "Method not allowed" }), { status: 405 });
  }
  try {
    const { candy_id } = await req.json();
    const price = CANDY_PRICES[candy_id];
    if (!price) {
      return new Response(JSON.stringify({ error: "Invalid candy_id" }), { status: 400 });
    }
    const supabase = createClient(
      Deno.env.get("SUPABASE_URL")!,
      Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!
    );
    const { data: txn, error: insertError } = await supabase
      .from("vending_transactions")
      .insert({ candy_id, amount_inr: price, status: "pending" })
      .select()
      .single();
    if (insertError) throw insertError;

    const razorpayKeyId = Deno.env.get("RAZORPAY_KEY_ID")!;
    const razorpayKeySecret = Deno.env.get("RAZORPAY_KEY_SECRET")!;
    const auth = btoa(`${razorpayKeyId}:${razorpayKeySecret}`);
    const orderRes = await fetch("https://api.razorpay.com/v1/orders", {
      method: "POST",
      headers: { "Authorization": `Basic ${auth}`, "Content-Type": "application/json" },
      body: JSON.stringify({ amount: price * 100, currency: "INR", receipt: txn.id }),
    });
    const order = await orderRes.json();
    if (!orderRes.ok) {
      await supabase.from("vending_transactions").update({ status: "failed" }).eq("id", txn.id);
      return new Response(JSON.stringify({ error: "Razorpay error", details: order }), { status: 502 });
    }
    await supabase.from("vending_transactions").update({ razorpay_order_id: order.id }).eq("id", txn.id);
    const checkoutUrl = `${Deno.env.get("SUPABASE_URL")}/functions/v1/checkout?id=${txn.id}`;
    return new Response(
      JSON.stringify({ transaction_id: txn.id, checkout_url: checkoutUrl, amount: price }),
      { headers: { "Content-Type": "application/json" } }
    );
  } catch (err) {
    return new Response(JSON.stringify({ error: String(err) }), { status: 500 });
  }
});
