import "jsr:@supabase/functions-js/edge-runtime.d.ts";
import { createClient } from "jsr:@supabase/supabase-js@2";

const cors={"Access-Control-Allow-Origin":"*","Access-Control-Allow-Headers":"content-type, apikey, authorization","Access-Control-Allow-Methods":"GET, PATCH, OPTIONS"};

Deno.serve(async(req:Request)=>{
 if(req.method==="OPTIONS") return new Response("ok",{headers:cors});
 const id=new URL(req.url).searchParams.get("id");
 if(!id) return Response.json({error:"Missing id"},{status:400,headers:cors});
 const keys=Deno.env.get("SUPABASE_SECRET_KEYS");
 const serviceKey=Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") ?? (keys ? JSON.parse(keys).default : null);
 if(!serviceKey) return Response.json({error:"Supabase server key is not configured"},{status:500,headers:cors});
 const supabase=createClient(Deno.env.get("SUPABASE_URL")!,serviceKey);
 if(req.method==="GET"){
  const {data,error}=await supabase.from("vending_transactions").select("id,status,candy_id,amount_inr").eq("id",id).single();
  if(error) return Response.json({error:"Transaction not found"},{status:404,headers:cors});
  return Response.json(data,{headers:cors});
 }
 if(req.method==="PATCH"){
  const {data,error}=await supabase.from("vending_transactions").update({status:"dispensed",dispensed_at:new Date().toISOString()}).eq("id",id).eq("status","paid").select("id,status,candy_id,amount_inr").single();
  if(error) return Response.json({error:"Transaction is not ready to dispense"},{status:409,headers:cors});
  return Response.json(data,{headers:cors});
 }
 return Response.json({error:"Method not allowed"},{status:405,headers:cors});
});
