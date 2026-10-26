import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

async function valid(body:string,sig:string,secret:string){
 const key=await crypto.subtle.importKey("raw",new TextEncoder().encode(secret),{name:"HMAC",hash:"SHA-256"},false,["sign"]);
 const buf=await crypto.subtle.sign("HMAC",key,new TextEncoder().encode(body));
 const expected=Array.from(new Uint8Array(buf)).map(b=>b.toString(16).padStart(2,"0")).join("");
 return expected===sig;
}

Deno.serve(async(req:Request)=>{
 if(req.method!=="POST") return Response.json({error:"Method not allowed"},{status:405});
 const body=await req.text(), sig=req.headers.get("x-razorpay-signature")??"", secret=Deno.env.get("RAZORPAY_WEBHOOK_SECRET");
 if(!secret) return Response.json({error:"Webhook secret not configured"},{status:500});
 if(!(await valid(body,sig,secret))) return Response.json({error:"Invalid signature"},{status:400});
 try{
  const event=JSON.parse(body);
  if(event.event!=="payment_link.paid") return Response.json({received:true});
  const link=event.payload?.payment_link?.entity, payment=event.payload?.payment?.entity;
  if(!link?.id||!payment?.id) return Response.json({received:true});
  const keys=Deno.env.get("SUPABASE_SECRET_KEYS");
  const serviceKey=Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") ?? (keys ? JSON.parse(keys).default : null);
  if(!serviceKey) throw new Error("Supabase server key is not configured");
  const supabase=createClient(Deno.env.get("SUPABASE_URL")!,serviceKey);
  const {error}=await supabase.from("vending_transactions").update({status:"paid",paid_at:new Date().toISOString(),razorpay_payment_id:payment.id}).eq("razorpay_order_id",link.id).eq("status","pending");
  if(error) throw error;
  return Response.json({received:true});
 }catch(e){return Response.json({error:String(e)},{status:500});}
});
