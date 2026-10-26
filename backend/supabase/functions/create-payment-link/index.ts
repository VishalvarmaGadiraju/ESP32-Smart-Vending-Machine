import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const PRICES: Record<number, number> = {1:5,2:10,3:20};
const cors={"Access-Control-Allow-Origin":"*","Access-Control-Allow-Headers":"content-type, apikey, authorization","Access-Control-Allow-Methods":"POST, OPTIONS"};

Deno.serve(async (req: Request) => {
 if(req.method==="OPTIONS") return new Response("ok",{headers:cors});
 if(req.method!=="POST") return Response.json({error:"Method not allowed"},{status:405,headers:cors});
 try{
  const {candy_id}=await req.json();
  const price=PRICES[Number(candy_id)];
  if(!price) return Response.json({error:"Invalid candy_id"},{status:400,headers:cors});
  const keys=Deno.env.get("SUPABASE_SECRET_KEYS");
  const serviceKey=Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") ?? (keys ? JSON.parse(keys).default : null);
  if(!serviceKey) throw new Error("Supabase server key is not configured");
  const supabase=createClient(Deno.env.get("SUPABASE_URL")!,serviceKey);
  const {data:txn,error}=await supabase.from("vending_transactions").insert({candy_id:Number(candy_id),amount_inr:price,status:"pending"}).select("id,candy_id,amount_inr,status").single();
  if(error) throw error;
  const keyId=Deno.env.get("RAZORPAY_KEY_ID"), keySecret=Deno.env.get("RAZORPAY_KEY_SECRET");
  if(!keyId||!keySecret) throw new Error("Razorpay test credentials are not configured");
  const response=await fetch("https://api.razorpay.com/v1/payment_links",{method:"POST",headers:{Authorization:"Basic "+btoa(keyId+":"+keySecret),"Content-Type":"application/json"},body:JSON.stringify({amount:price*100,currency:"INR",description:"Candy "+candy_id+" - Rs "+price,reference_id:txn.id,callback_method:"get"})});
  const link=await response.json();
  if(!response.ok){await supabase.from("vending_transactions").update({status:"failed"}).eq("id",txn.id);return Response.json({error:"Razorpay error",details:link},{status:502,headers:cors});}
  await supabase.from("vending_transactions").update({razorpay_order_id:link.id}).eq("id",txn.id);
  return Response.json({transaction_id:txn.id,payment_url:link.short_url,amount:price,candy_id:Number(candy_id)},{headers:cors});
 }catch(e){return Response.json({error:String(e)},{status:500,headers:cors});}
});
