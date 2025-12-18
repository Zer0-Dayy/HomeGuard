import json
import re
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional
import os
import pandas as pd
import requests
import streamlit as st


def get_ollama_base_url():
    return os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")



def set_ollama_base_url(url: str) -> None:
    st.session_state["ollama_base_url"] = url.rstrip("/")


@dataclass
class ModelAnswer:
    model: str
    response: str
    created_at: datetime


@dataclass
class ScoredAnswer:
    answer_model: str
    judge_model: str
    score: float
    reasoning: str
    raw_response: str
    created_at: datetime


def fetch_models(base_url: str) -> List[str]:
    response = requests.get(f"{base_url}/api/tags", timeout=10)
    response.raise_for_status()
    payload = response.json()
    models = payload.get("models", [])
    return [model_info.get("name", "") for model_info in models if model_info.get("name")]


def call_ollama(model: str, prompt: str, base_url: str, system_prompt: Optional[str] = None) -> str:
    payload: Dict[str, object] = {
        "model": model,
        "prompt": prompt,
        "stream": False,
    }
    if system_prompt:
        payload["system"] = system_prompt
    response = requests.post(f"{base_url}/api/generate", json=payload, timeout= None)
    response.raise_for_status()
    data = response.json()
    return data.get("response", "").strip()


def build_scoring_prompt(criteria: str, answer: str) -> str:
    criteria_block = criteria.strip() or "No additional criteria provided."
    return (
        "You are evaluating a model answer. Read the criteria and return a JSON object with "
        "a numeric 'score' (1-10) and a short 'reason'. Do not add any extra text.\n"
        f"Criteria:\n{criteria_block}\n\n"
        "Answer to score:\n"
        f"{answer}\n\n"
        "Respond with JSON like {\"score\": 8.5, \"reason\": \"...\"}."
    )



def parse_score(text: str) -> Optional[float]:
    cleaned = text.strip()

    # 1) Strip Markdown code fences like ```json ... ```
    cleaned = re.sub(r"^```[\w]*", "", cleaned)
    cleaned = re.sub(r"```$", "", cleaned)
    cleaned = cleaned.strip()

    # 2) Try extracting the JSON object even if wrapped in other text
    json_match = re.search(r"\{.*\}", cleaned, flags=re.DOTALL)
    if json_match:
        try:
            candidate = json.loads(json_match.group(0))
            if isinstance(candidate, dict) and "score" in candidate:
                score_value = candidate["score"]
                if isinstance(score_value, (int, float)):
                    return float(score_value)
        except json.JSONDecodeError:
            pass

    # 3) Fallback: Parse loose "score: X" or "score = X"
    match = re.search(
        r"score\s*[:=]\s*([0-9]+(?:\.[0-9]+)?)",
        cleaned,
        flags=re.IGNORECASE
    )
    if match:
        return float(match.group(1))

    return None

def ensure_directory(path: Path) -> None:
    if path.parent and not path.parent.exists():
        path.parent.mkdir(parents=True, exist_ok=True)


def save_answers(path: Path, answers: List[ModelAnswer]) -> None:
    ensure_directory(path)
    rows = [
        {
            "model": ans.model,
            "response": ans.response,
            "created_at": ans.created_at.isoformat(),
        }
        for ans in answers
    ]
    df = pd.DataFrame(rows)
    if path.exists():
        existing = pd.read_csv(path)
        df = pd.concat([existing, df], ignore_index=True)
    df.to_csv(path, index=False)


def save_scores(path: Path, scores: List[ScoredAnswer]) -> None:
    ensure_directory(path)
    rows = [
        {
            "answer_model": item.answer_model,
            "judge_model": item.judge_model,
            "score": item.score,
            "reasoning": item.reasoning,
            "raw_response": item.raw_response,
            "created_at": item.created_at.isoformat(),
        }
        for item in scores
    ]
    df = pd.DataFrame(rows)
    if path.exists():
        existing = pd.read_csv(path)
        df = pd.concat([existing, df], ignore_index=True)
    df.to_csv(path, index=False)


def score_answers(
    answers: List[ModelAnswer],
    judge_models: List[str],
    base_url: str,
    criteria: str,
    system_prompt: Optional[str],
) -> List[ScoredAnswer]:
    results: List[ScoredAnswer] = []
    for answer in answers:
        for judge in judge_models:
            scoring_prompt = build_scoring_prompt(criteria, answer.response)
            raw = call_ollama(judge, scoring_prompt, base_url, system_prompt)
            numeric = parse_score(raw)
            if numeric is None:
                numeric = float("nan")
            reasoning_match = re.search(r"reason\s*[:=]\s*(.*)", raw, flags=re.IGNORECASE)
            reasoning = reasoning_match.group(1).strip() if reasoning_match else raw
            results.append(
                ScoredAnswer(
                    answer_model=answer.model,
                    judge_model=judge,
                    score=numeric,
                    reasoning=reasoning,
                    raw_response=raw,
                    created_at=datetime.utcnow(),
                )
            )
    return results


def display_summary(scores: List[ScoredAnswer]) -> None:
    if not scores:
        st.info("No scores available yet.")
        return
    df = pd.DataFrame([
        {
            "answer_model": item.answer_model,
            "judge_model": item.judge_model,
            "score": item.score,
        }
        for item in scores
    ])
    summary = (
        df.groupby("answer_model")["score"].mean().reset_index().sort_values(by="score", ascending=False)
    )
    st.subheader("Average scores by answer model")
    st.dataframe(summary, use_container_width=True)


st.set_page_config(page_title="Local LLM Benchmark", layout="wide")
st.title("Local LLM Benchmark for Ollama")

if "answers" not in st.session_state:
    st.session_state["answers"] = []
if "scores" not in st.session_state:
    st.session_state["scores"] = []

with st.sidebar:
    st.header("Ollama connection")
    base_url_input = st.text_input("Base URL", value=get_ollama_base_url())
    if st.button("Save Ollama URL"):
        set_ollama_base_url(base_url_input)
        st.success("Base URL updated.")
    base_url = get_ollama_base_url()

    st.markdown("---")
    st.header("Model selection")
    if st.button("Refresh available models"):
        try:
            st.session_state["available_models"] = fetch_models(base_url)
        except Exception as exc:  # noqa: BLE001
            st.error(f"Could not fetch models: {exc}")
    available_models = st.session_state.get("available_models", [])
    selected_models = st.multiselect(
        "Models to benchmark",
        options=available_models,
        help="Pick the models that will generate answers and also judge them.",
    )

st.subheader("1) Provide task and criteria")
col1, col2 = st.columns(2)
with col1:
    task_prompt = st.text_area(
        "Task prompt (system/instruction)",
        help="What should the models answer?",
        height=180,
    )
with col2:
    evaluation_criteria = st.text_area(
        "Evaluation criteria",
        help="How should answers be judged? Provide bullet points, rubrics, or scales.",
        height=180,
    )

answer_path = st.text_input("Save answers to (CSV)", value="data/model_answers.csv")
score_path = st.text_input("Save scores to (CSV)", value="data/model_scores.csv")

st.markdown("---")

st.subheader("2) Generate answers")
run_benchmark = st.button("Run benchmark", disabled=not task_prompt or not selected_models)
if run_benchmark:
    try:
        answers: List[ModelAnswer] = []
        for model in selected_models:
            response = call_ollama(model, task_prompt, base_url)
            answers.append(ModelAnswer(model=model, response=response, created_at=datetime.utcnow()))
        st.session_state["answers"].extend(answers)
        save_answers(Path(answer_path), answers)
        st.success(f"Captured {len(answers)} answers and saved to {answer_path}.")
    except Exception as exc:  # noqa: BLE001
        st.error(f"Failed to run benchmark: {exc}")

if st.session_state["answers"]:
    st.write("Collected answers")
    answers_df = pd.DataFrame([
        {"model": ans.model, "created_at": ans.created_at, "response": ans.response}
        for ans in st.session_state["answers"]
    ])
    st.dataframe(answers_df, use_container_width=True)

st.markdown("---")

st.subheader("3) Score answers with selected models")
eval_system_prompt = st.text_area(
    "Optional evaluator system prompt",
    value=(
        "You are a strict evaluator. Only respond with JSON containing a numeric score (1-10) "
        "and a brief reason."
    ),
)
score_answers_btn = st.button(
    "Score collected answers",
    disabled=not st.session_state["answers"] or not evaluation_criteria or not selected_models,
)

if score_answers_btn:
    try:
        scores = score_answers(
            answers=st.session_state["answers"],
            judge_models=selected_models,
            base_url=base_url,
            criteria=evaluation_criteria,
            system_prompt=eval_system_prompt,
        )
        st.session_state["scores"].extend(scores)
        save_scores(Path(score_path), scores)
        st.success(f"Scored answers with {len(selected_models)} judge models.")
    except Exception as exc:  # noqa: BLE001
        st.error(f"Failed to score answers: {exc}")

if st.session_state["scores"]:
    st.write("Individual scoring details")
    scores_df = pd.DataFrame([
        {
            "answer_model": item.answer_model,
            "judge_model": item.judge_model,
            "score": item.score,
            "reasoning": item.reasoning,
            "created_at": item.created_at,
        }
        for item in st.session_state["scores"]
    ])
    st.dataframe(scores_df, use_container_width=True)
    display_summary(st.session_state["scores"])

st.markdown(
    """
    ### How it works
    1. Refresh and pick the Ollama models you want to benchmark.
    2. Enter the task prompt and evaluation criteria.
    3. Run the benchmark to collect each model's answer.
    4. Re-use the same models (or a subset) as judges to score the answers.
    5. Download CSVs from the provided paths for further analysis.
    """
)
